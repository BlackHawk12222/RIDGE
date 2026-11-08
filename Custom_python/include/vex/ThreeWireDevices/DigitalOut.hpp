#ifndef DIGITAL_OUT_HPP
#define DIGITAL_OUT_HPP

#include "pros/adi.hpp"

class DigitalOut{
    private:
        pros::adi::DigitalOut* Output = nullptr;

    public:
        explicit DigitalOut(int port);

        void set(bool value);
};

#endif // DIGITAL_OUT_HPP