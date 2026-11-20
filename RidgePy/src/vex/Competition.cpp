#include "pros/misc.hpp"
#include "vex/Competition.hpp"

bool VexCompetition::is_autonomous() {
    return pros::competition::is_autonomous();
}

bool VexCompetition::is_driver_control() {
    return is_enabled() && !is_autonomous();
}

bool VexCompetition::is_enabled() {
    return !pros::competition::is_disabled();
}

bool VexCompetition::is_disabled() {
    return pros::competition::is_disabled();
}

bool VexCompetition::is_competition_switch() {
    return pros::competition::is_competition_switch();
}

bool VexCompetition::is_field_control() {
    return pros::competition::is_field_control();
}

extern "C" bool vex_competition_is_autonomous() {
    return VexCompetition::is_autonomous();
}

extern "C" bool vex_competition_is_driver_control() {
    return VexCompetition::is_driver_control();
}

extern "C" bool vex_competition_is_enabled() {
    return VexCompetition::is_enabled();
}

extern "C" bool vex_competition_is_disabled() {
    return VexCompetition::is_disabled();
}

extern "C" bool vex_competition_is_competition_switch() {
    return VexCompetition::is_competition_switch();
}

extern "C" bool vex_competition_is_field_control() {
    return VexCompetition::is_field_control();
}
