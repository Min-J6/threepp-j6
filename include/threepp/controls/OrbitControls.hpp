
#ifndef THREEPP_ORBITCONTROLS_HPP
#define THREEPP_ORBITCONTROLS_HPP

#include "threepp/math/MathUtils.hpp"
#include "threepp/math/Vector3.hpp"

#include <limits>
#include <memory>

namespace threepp {

    class PeripheralsEventSource;

    class OrbitControls {

    public:
        bool enabled = true;

        Vector3 target;

        float minDistance = 0.f;
        float maxDistance = std::numeric_limits<float>::infinity();

        float minZoom = 0.f;
        float maxZoom = std::numeric_limits<float>::infinity();

        float minPolarAngle = 0.f;
        float maxPolarAngle = math::PI;

        // How far you can orbit horizontally, upper and lower limits.
        // If set, must be a sub-interval of the interval [ - Math.PI, Math.PI ].
        float minAzimuthAngle = -std::numeric_limits<float>::infinity();// radians
        float maxAzimuthAngle = std::numeric_limits<float>::infinity(); // radians

        // Set to true to enable damping (inertia)
        // If damping is enabled, you must call controls.update() in your animation loop
        bool enableDamping = false;
        float dampingFactor = 0.05f;

        // This option actually enables dollying in and out; left as "zoom" for backwards compatibility.
        // Set to false to disable zooming
        bool enableZoom = true;
        float zoomSpeed = 1.0f;

        // Set to false to disable rotating
        bool enableRotate = true;
        float rotateSpeed = 1.0f;

        // Set to false to disable panning
        bool enablePan = true;
        float panSpeed = 1.0f;
        bool screenSpacePanning = true;// if true, pan in screen-space
        float keyPanSpeed = 7.f;       // pixels moved per arrow key push

        // Set to true to automatically rotate around the target
        // If auto-rotate is enabled, you must call controls.update() in your animation loop
        bool autoRotate = false;
        float autoRotateSpeed = 2.0f;// 30 seconds per round when fps is 60

        // Set to false to disable use of the keys
        bool enableKeys = true;

        OrbitControls(Camera& camera, PeripheralsEventSource& eventSource);

        bool update();

        [[nodiscard]] float getAutoRotationAngle() const;

        [[nodiscard]] float getZoomScale() const;

        ~OrbitControls();

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl_;


    // Custom code by jusik
    public:
        // Movement properties
        float moveSpeed = 0.031415f;
        float acceleration = 1.0f;     // How quickly to reach target speed
        float deceleration = 20.0f;     // How quickly to slow down
        float maxSpeed = 1.f;        // Maximum movement speed

        // Current velocity for smooth movement
        Vector3 currentVelocity{0, 0, 0};
        Vector3 targetVelocity{0, 0, 0};
        float deltaTime = 0.016f;      // Assume 60fps, adjust as needed

        void moveOnPlane(char direction);

        void moveForward(float distance);

        // Rotate camera based on mouse drag
        void rotateCamera(float deltaX, float deltaY);

        // Zoom camera based on mouse wheel
        void zoomCamera(float zoomDelta);

        // Pan camera based on mouse drag
        void panCamera(float deltaX, float deltaY);

        void handleWASDMovement(bool isWPressed, bool isSPressed, bool isAPressed, bool isDPressed);

        // Update camera based on mouse input
        void updateFromMouseInput(float deltaX, float deltaY, float wheelDelta,
                            bool isMiddleMouseDown, bool isShiftDown, bool isCtrlDown,
                            bool isWPressed, bool isSPressed, bool isAPressed, bool isDPressed);
    };

}// namespace threepp

#endif//THREEPP_ORBITCONTROLS_HPP
