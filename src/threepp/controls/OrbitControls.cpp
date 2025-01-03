
#include "threepp/controls/OrbitControls.hpp"

#include "threepp/input/PeripheralsEventSource.hpp"
#include "threepp/math/Spherical.hpp"

#include "threepp/cameras/OrthographicCamera.hpp"
#include "threepp/cameras/PerspectiveCamera.hpp"

#include <cmath>
#include <iostream>

using namespace threepp;

namespace {

    const float EPS = 0.000001f;

    enum State {

        NONE,
        ROTATE,
        DOLLY,
        PAN,

    };


}// namespace

struct OrbitControls::Impl {

    OrbitControls& scope;
    PeripheralsEventSource& canvas;
    Camera& camera;

    std::unique_ptr<KeyListener> keyListener;
    std::unique_ptr<MouseListener> mouseListener;

    State state = NONE;

    // current position in spherical coordinates
    Spherical spherical;
    Spherical sphericalDelta;

    float scale = 1.f;
    Vector3 panOffset;
    bool zoomChanged = false;

    Vector2 rotateStart;
    Vector2 rotateEnd;
    Vector2 rotateDelta;

    Vector2 panStart;
    Vector2 panEnd;
    Vector2 panDelta;

    Vector2 dollyStart;
    Vector2 dollyEnd;
    Vector2 dollyDelta;

    Impl(OrbitControls& scope, PeripheralsEventSource& canvas, Camera& camera)
        : scope(scope), canvas(canvas), camera(camera),
          keyListener(std::make_unique<MyKeyListener>(scope)),
          mouseListener(std::make_unique<MyMouseListener>(scope)) {

        // Custom code by jusik
        // canvas.addMouseListener(*mouseListener);
        // canvas.addKeyListener(*keyListener);

        update();
    }

    bool update() {

        Vector3 offset;

        // so camera.up is the orbit axis
        const auto quat = Quaternion().setFromUnitVectors(camera.up, {0, 1, 0});
        auto quatInverse = Quaternion().copy(quat).invert();

        Vector3 lastPosition;
        Quaternion lastQuaternion;

        auto& position = this->camera.position;

        offset.copy(position).sub(scope.target);

        // rotate offset to "y-axis-is-up" space
        offset.applyQuaternion(quat);

        // angle from z-axis around y-axis
        spherical.setFromVector3(offset);

        if (scope.autoRotate && state == State::NONE) {

            rotateLeft(scope.getAutoRotationAngle());
        }

        if (scope.enableDamping) {

            spherical.theta += sphericalDelta.theta * scope.dampingFactor;
            spherical.phi += sphericalDelta.phi * scope.dampingFactor;

        } else {

            spherical.theta += sphericalDelta.theta;
            spherical.phi += sphericalDelta.phi;
        }

        // restrict theta to be between desired limits
        spherical.theta = std::max(scope.minAzimuthAngle, std::min(scope.maxAzimuthAngle, spherical.theta));

        // restrict phi to be between desired limits
        spherical.phi = std::max(scope.minPolarAngle, std::min(scope.maxPolarAngle, spherical.phi));

        spherical.makeSafe();

        spherical.radius *= scale;

        // restrict radius to be between desired limits
        spherical.radius = std::max(scope.minDistance, std::min(scope.maxDistance, spherical.radius));

        // move target to panned location

        if (scope.enableDamping) {

            scope.target.addScaledVector(panOffset, scope.dampingFactor);

        } else {

            scope.target.add(panOffset);
        }

        offset.setFromSpherical(spherical);

        // rotate offset back to "camera-up-vector-is-up" space
        offset.applyQuaternion(quatInverse);

        position.copy(scope.target).add(offset);

        this->camera.lookAt(scope.target);

        if (scope.enableDamping) {

            sphericalDelta.theta *= (1 - scope.dampingFactor);
            sphericalDelta.phi *= (1 - scope.dampingFactor);

            panOffset.multiplyScalar(1 - scope.dampingFactor);

        } else {

            sphericalDelta.set(0, 0, 0);

            panOffset.set(0, 0, 0);
        }

        scale = 1;

        // update condition is:
        // min(camera displacement, camera rotation in radians)^2 > EPS
        // using small-angle approximation cos(x/2) = 1 - x^2 / 8

        if (zoomChanged ||
            lastPosition.distanceToSquared(this->camera.position) > EPS ||
            8 * (1 - lastQuaternion.dot(this->camera.quaternion)) > EPS) {

            lastPosition.copy(this->camera.position);
            lastQuaternion.copy(this->camera.quaternion);
            zoomChanged = false;

            return true;
        }

        return false;
    }

    void rotateLeft(float angle) {

        sphericalDelta.theta -= angle;
    }

    void rotateUp(float angle) {

        sphericalDelta.phi -= angle;
    }

    void panLeft(float distance, const Matrix4& objectMatrix) {

        Vector3 v;

        v.setFromMatrixColumn(objectMatrix, 0);// get X column of objectMatrix
        v.multiplyScalar(-distance);

        panOffset.add(v);
    }

    void panUp(float distance, const Matrix4& objectMatrix) {

        Vector3 v;

        if (scope.screenSpacePanning) {

            v.setFromMatrixColumn(objectMatrix, 1);

        } else {

            v.setFromMatrixColumn(objectMatrix, 0);
            v.crossVectors(this->camera.up, v);
        }

        v.multiplyScalar(distance);

        panOffset.add(v);
    }

    void pan(float deltaX, float deltaY) {

        Vector3 offset;

        if (auto perspective = camera.as<PerspectiveCamera>()) {

            // perspective
            auto& position = this->camera.position;
            offset.copy(position).sub(scope.target);
            auto targetDistance = offset.length();

            // half of the fov is center to top of screen
            targetDistance *= std::tan((perspective->fov / 2) * math::PI / 180.f);

            // we use only clientHeight here so aspect ratio does not distort speed
            const auto size = canvas.size();
            panLeft(2 * deltaX * targetDistance / static_cast<float>(size.height()), *this->camera.matrix);
            panUp(2 * deltaY * targetDistance / static_cast<float>(size.height()), *this->camera.matrix);
        } else if (auto ortho = camera.as<OrthographicCamera>()) {

            const auto size = canvas.size();

            // orthographic
            panLeft(
                    deltaX * (ortho->right - ortho->left) / this->camera.zoom / static_cast<float>(size.width()),
                    *this->camera.matrix);
            panUp(
                    deltaY * (ortho->top - ortho->bottom) / this->camera.zoom / static_cast<float>(size.height()),
                    *this->camera.matrix);

        } else {

            // camera neither orthographic nor perspective
            std::cerr << "[OrbitControls] encountered an unknown camera type - pan disabled." << std::endl;
            scope.enablePan = false;
        }
    }

    void dollyIn(float dollyScale) {

        if (camera.as<PerspectiveCamera>()) {

            scale /= dollyScale;

        } else if (camera.as<OrthographicCamera>()) {

            this->camera.zoom = std::max(scope.minZoom, std::min(scope.maxZoom, this->camera.zoom * dollyScale));
            this->camera.updateProjectionMatrix();
            zoomChanged = true;
        } else {

            std::cerr << "[OrbotControls] encountered an unknown camera type - dolly/zoom disabled." << std::endl;
            scope.enableZoom = false;
        }
    }

    void dollyOut(float dollyScale) {

        if (camera.as<PerspectiveCamera>()) {
            scale *= dollyScale;
        } else if (camera.as<OrthographicCamera>()) {
            this->camera.zoom = std::max(scope.minZoom, std::min(scope.maxZoom, this->camera.zoom / dollyScale));
            this->camera.updateProjectionMatrix();
            zoomChanged = true;
        } else {
            std::cerr << "[OrbotControls] encountered an unknown camera type - dolly/zoom disabled." << std::endl;
            scope.enableZoom = false;
        }
    }


    void handleKeyDown(Key key) {

        bool needsUpdate = true;

        switch (key) {

            case Key::UP:
                pan(0, scope.keyPanSpeed);
                break;
            case Key::DOWN:
                pan(0, -scope.keyPanSpeed);
                break;
            case Key::LEFT:
                pan(scope.keyPanSpeed, 0);
                break;
            case Key::RIGHT:
                pan(-scope.keyPanSpeed, 0);
                break;
            default:
                needsUpdate = false;
                break;
        }

        if (needsUpdate) {
            scope.update();
        }
    }

    void handleMouseDownRotate(const Vector2& pos) {

        rotateStart.copy(pos);
    }

    void handleMouseDownDolly(const Vector2& pos) {

        dollyStart.copy(pos);
    }

    void handleMouseDownPan(const Vector2& pos) {

        panStart.copy(pos);
    }

    void handleMouseMoveRotate(const Vector2& pos) {

        rotateEnd.copy(pos);

        rotateDelta.subVectors(rotateEnd, rotateStart).multiplyScalar(scope.rotateSpeed);

        const auto size = canvas.size();
        rotateLeft(2 * math::PI * rotateDelta.x / static_cast<float>(size.height()));// yes, height

        rotateUp(2 * math::PI * rotateDelta.y / static_cast<float>(size.height()));

        rotateStart.copy(rotateEnd);

        scope.update();
    }

    void handleMouseMoveDolly(const Vector2& pos) {

        dollyEnd.copy(pos);

        dollyDelta.subVectors(dollyEnd, dollyStart);

        if (dollyDelta.y > 0) {

            dollyIn(scope.getZoomScale());

        } else if (dollyDelta.y < 0) {

            dollyOut(scope.getZoomScale());
        }

        dollyStart.copy(dollyEnd);

        scope.update();
    }


    void handleMouseMovePan(const Vector2& pos) {

        panEnd.copy(pos);

        panDelta.subVectors(panEnd, panStart).multiplyScalar(scope.panSpeed);

        pan(panDelta.x, panDelta.y);

        panStart.copy(panEnd);

        scope.update();
    }

    void handleMouseWheel(const Vector2& delta) {
        if (delta.y < 0) {

            dollyIn(scope.getZoomScale());

        } else if (delta.y > 0) {

            dollyOut(scope.getZoomScale());
        }

        scope.update();
    }

    ~Impl() {

        canvas.removeMouseListener(*mouseListener);
        canvas.removeKeyListener(*keyListener);
    }

    struct MyKeyListener: KeyListener {

        OrbitControls& controls;

        explicit MyKeyListener(OrbitControls& controls)
            : controls(controls) {}

        void onKeyPressed(KeyEvent evt) override {
            if (controls.enabled && controls.enableKeys && controls.enablePan) {
                controls.pimpl_->handleKeyDown(evt.key);
            }
        }

        void onKeyRepeat(KeyEvent evt) override {
            if (controls.enabled && controls.enableKeys && controls.enablePan) {
                controls.pimpl_->handleKeyDown(evt.key);
            }
        }
    };

    struct MyMouseMoveListener: MouseListener {

        OrbitControls& controls;

        explicit MyMouseMoveListener(OrbitControls& controls): controls(controls) {}

        void onMouseMove(const Vector2& pos) override {
            if (controls.enabled) {

                switch (controls.pimpl_->state) {
                    case State::ROTATE:
                        if (controls.enableRotate) {
                            controls.pimpl_->handleMouseMoveRotate(pos);
                        }
                        break;
                    case State::DOLLY:
                        if (controls.enableZoom) {
                            controls.pimpl_->handleMouseMoveDolly(pos);
                        }
                        break;
                    case State::PAN:
                        if (controls.enablePan) {
                            controls.pimpl_->handleMouseMovePan(pos);
                        }
                        break;
                    default:
                        //TODO ?
                        break;
                }
            }
        }
    };


    struct MyMouseUpListener: MouseListener {

        OrbitControls& scope;
        MouseListener* mouseMoveListener;

        MyMouseUpListener(OrbitControls& scope, MyMouseMoveListener* mouseMoveListener)
            : scope(scope), mouseMoveListener(mouseMoveListener) {}

        void onMouseUp(int, const Vector2&) override {
            if (scope.enabled) {

                scope.pimpl_->canvas.removeMouseListener(*mouseMoveListener);
                scope.pimpl_->canvas.removeMouseListener(*this);
                scope.pimpl_->state = NONE;
            }
        }
    };

    struct MyMouseListener: MouseListener {

        OrbitControls& scope;
        MyMouseMoveListener mouseMoveListener;
        MyMouseUpListener mouseUpListener;

        explicit MyMouseListener(OrbitControls& scope)
            : scope(scope), mouseMoveListener(scope), mouseUpListener(scope, &mouseMoveListener) {}

        void onMouseDown(int button, const Vector2& pos) override {
            if (scope.enabled) {
                switch (button) {
                    case 0:// LEFT
                        if (scope.enableRotate) {
                            scope.pimpl_->handleMouseDownRotate(pos);
                            scope.pimpl_->state = State::ROTATE;
                        }
                        break;
                    case 1:// RIGHT
                        if (scope.enablePan) {
                            scope.pimpl_->handleMouseDownRotate(pos);
                            scope.pimpl_->handleMouseDownPan(pos);
                            scope.pimpl_->state = State::PAN;
                        }
                        break;
                    case 2:// MIDDLE
                        if (scope.enableZoom) {
                            scope.pimpl_->handleMouseDownDolly(pos);
                            scope.pimpl_->state = State::DOLLY;
                        }
                        break;
                }
            }

            if (scope.pimpl_->state != State::NONE) {

                scope.pimpl_->canvas.addMouseListener(mouseMoveListener);
                scope.pimpl_->canvas.addMouseListener(mouseUpListener);
            }
        }

        void onMouseWheel(const Vector2& delta) override {
            if (scope.enabled && scope.enableZoom && !(scope.pimpl_->state != State::NONE && scope.pimpl_->state != State::ROTATE)) {
                scope.pimpl_->handleMouseWheel(delta);
            }
        }
    };
};

OrbitControls::OrbitControls(Camera& camera, PeripheralsEventSource& eventSource)
    : pimpl_(std::make_unique<Impl>(*this, eventSource, camera)) {}


bool OrbitControls::update() {

    return pimpl_->update();
}

float OrbitControls::getAutoRotationAngle() const {

    return 2 * math::PI / 60 / 60 * this->autoRotateSpeed;
}

float OrbitControls::getZoomScale() const {

    return std::pow(0.95f, this->zoomSpeed);
}

OrbitControls::~OrbitControls() = default;

// Rotate camera based on mouse drag
void OrbitControls::rotateCamera(float deltaX, float deltaY) {
    if (enabled && enableRotate) {
        float rotateSpeed = 0.005f;  // Adjust this value to control rotation speed
        pimpl_->rotateLeft(deltaX * rotateSpeed);
        pimpl_->rotateUp(deltaY * rotateSpeed);
        update();
    }
}

// Zoom camera based on mouse wheel
void OrbitControls::zoomCamera(float zoomDelta) {
    if (enabled && enableZoom) {
        if (zoomDelta > 0) {
            pimpl_->dollyIn(getZoomScale());
        } else if (zoomDelta < 0) {
            pimpl_->dollyOut(getZoomScale());
        }
        update();
    }
}

// Pan camera based on mouse drag
void OrbitControls::panCamera(float deltaX, float deltaY) {
    if (enabled && enablePan) {
        float panSpeed = 1.0f;  // Adjust this value to control pan speed
        pimpl_->pan(deltaX * panSpeed, deltaY * panSpeed);
        update();
    }
}

// Update camera based on mouse input
void OrbitControls::updateFromMouseInput(float deltaX, float deltaY, float wheelDelta,
                                       bool isMiddleMouseDown, bool isShiftDown, bool isCtrlDown,
                                       bool isWPressed, bool isSPressed, bool isAPressed, bool isDPressed) {
    // 기존 마우스 컨트롤 처리
    if (isMiddleMouseDown) {
        if (isShiftDown) {
            // Shift + 휠클릭 드래그 = Pan
            panCamera(deltaX, deltaY);
        }
        else if (isCtrlDown) {
            // Ctrl + 휠클릭 드래그 = Zoom
            zoomCamera(-deltaY * 0.01f);
        }
        else {
            // 휠클릭 드래그 = Rotate
            rotateCamera(deltaX, deltaY);
        }
    }

    // 휠 스크롤 처리
    if (wheelDelta != 0.0f) {
        zoomCamera(-wheelDelta * 0.1f);
    }

    // WASD 이동 처리
    handleWASDMovement(isWPressed, isSPressed, isAPressed, isDPressed);
}

void OrbitControls::moveForward(float distance) {
    if (enabled) {
        // 카메라의 앞 방향 벡터 계산 (타겟 방향)
        Vector3 forward;
        forward.subVectors(target, pimpl_->camera.position);
        forward.normalize();

        // 카메라와 타겟을 같은 방향으로 이동
        Vector3 movement;
        movement.copy(forward).multiplyScalar(distance);

        pimpl_->camera.position.add(movement);
        target.add(movement);

        update();
    }
}

void OrbitControls::moveOnPlane(char direction) {
    if (!enabled) return;

    Vector3 movement;
    float delta = moveSpeed;

    // Get the forward and right vectors in the XZ plane
    Vector3 forward;
    forward.subVectors(target, pimpl_->camera.position);
    forward.y = 0;  // Project onto XZ plane
    forward.normalize();

    Vector3 right;
    right.crossVectors(Vector3(0, 1, 0), forward);
    right.normalize();

    switch (direction) {
        case 'W':
            movement.copy(forward).multiplyScalar(delta);
        break;
        case 'S':
            movement.copy(forward).multiplyScalar(-delta);
        break;
        case 'D':
            movement.copy(right).multiplyScalar(delta);
        break;
        case 'A':
            movement.copy(right).multiplyScalar(-delta);
        break;
    }

    // Move both camera and target to maintain the same view direction
    pimpl_->camera.position.add(movement);
    target.add(movement);

    update();
}


void OrbitControls::handleWASDMovement(bool isWPressed, bool isSPressed, bool isAPressed, bool isDPressed) {
    if (!enabled) return;

    // 카메라의 전방 벡터 계산 (z 컴포넌트는 0으로 하여 X-Y 평면상의 이동만 처리)
    Vector3 forward;
    forward.subVectors(target, pimpl_->camera.position);
    forward.z = 0;  // X-Y 평면에 투영
    forward.normalize();

    // 오른쪽 벡터 계산 (Z가 up이므로 (0,0,1)과 외적)
    Vector3 right;
    right.crossVectors(forward, Vector3(0, 0, 1));
    right.normalize();

    // 목표 이동 방향 계산
    Vector3 targetDirection;

    if (isWPressed) targetDirection.add(forward);
    if (isSPressed) targetDirection.sub(forward);
    if (isDPressed) targetDirection.add(right);
    if (isAPressed) targetDirection.sub(right);

    // 만약 이동 입력이 있다면
    if (targetDirection.length() > 0) {
        // 이동 방향 정규화하고 속도 적용
        targetDirection.normalize();
        targetDirection.multiplyScalar(moveSpeed);

        // 현재 속도를 목표 방향으로 부드럽게 보간
        currentVelocity.lerp(targetDirection, acceleration * deltaTime);
    } else {
        // 입력이 없으면 감속
        currentVelocity.multiplyScalar(1.0f - deceleration * deltaTime);
    }

    // 매우 작은 속도는 0으로 만들어 미세한 움직임 방지
    if (currentVelocity.length() < 0.0001f) {
        currentVelocity.set(0, 0, 0);
    }

    // 현재 속도로 이동
    if (currentVelocity.length() > 0) {
        // 카메라와 타겟 모두 이동
        pimpl_->camera.position.add(currentVelocity);
        target.add(currentVelocity);
        update();
    }
}