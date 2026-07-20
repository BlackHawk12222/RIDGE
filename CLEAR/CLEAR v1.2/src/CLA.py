    # ---------------------------------------------------------------------------- #
    #                                                                              #
    # 	Module:       CLEAR.py                                                     #
    # 	Author:       Micah Bow                                                    #
    # 	Created:      1/27/2026, 12:42 PM                                          #
    #   Last Edited:  3/22/2026, 2:00 PM                                           #
    # 	Description:  Capture, Logging, Encoding, Archiving, Recording.            #
    #                                                                              #
    # ---------------------------------------------------------------------------- #

_errorcreated=False
from vex import *
import sys
brain=Brain()

try:
    from gc import collect, mem_alloc# type: ignore 
    from ustruct import pack_into
    from micropython import const, mem_info  # type: ignore

    
    log_time= Timer() # Main timer used.
    log_link=MessageLink(Ports.PORT21, "CLEAR32449", VexlinkType.MANAGER)

    def none():
        pass
    
    class Capture:
            """Main object for capturing data of the robot and seeing if it needs to be logged."""
            class System:
                """Main object" for general program data like memory use ."""

                def __init__(self):
                    self.modulelist={}
                    self.memory: float=0
                    self.currentMemory: float=0
                    self.memory_tolrance: int=const(int(str(settings.settings.get('memory_tolrance_KB '))))
                    self.aton: bool=False
                    self.driver: bool=False
                    self.comp_switch: bool=False
                    self.field: bool=False

                def memoryuse(self) -> None:
                    self.currentMemory: float=mem_alloc()/1000  # type: ignore
                    if not (self.memory >= self.currentMemory - self.memory_tolrance and self.memory <= self.currentMemory + self.memory_tolrance):
                        log.add(log.memory_change, str(self.currentMemory) + " KB")
                        self.memory: float=self.currentMemory
                
                def modules(self) -> None:
                    if self.modulelist != sys.modules:
                        filtered_list = [item for item in sys.modules if item not in self.modulelist]
                        log.add(log.module_change, filtered_list)
                        self.modulelist= sys.modules.copy()

                def control(self, comp: Competition):
                    if comp.is_autonomous() and not self.aton:
                        log.add(log.aton_start, "")
                        self.aton=True
                        self.driver=False
                    elif comp.is_driver_control() and not self.driver:
                        log.add("DSC1", "")
                        self.driver=True
                        self.aton=False
                    elif comp.is_competition_switch() and not self.comp_switch:
                        log.add("DSC2", "")
                        self.comp_switch=True
                    elif not comp.is_field_control() and self.field:
                        log.add("DSC5", "")
                        self.field=False
                    elif comp.is_field_control() and not self.field:
                        log.add("DSC3", "")
                        self.field=True
                    elif not comp.is_field_control() and self.field:
                        log.add("DSC4", "")
                        self.field=False

            class Smartport:
                def __init__(self):
                    # Sets for other motors By there id.
                    self.motor_temp_monitoring={} 
                    self.motor_power_monitoring={}  
                    self.motor_disconnected={}
                    self.motor_current_monitoring={}
                    self.motor_name={}
                    self.setup: dict[int, bool] = {}
                    self.motor_id=0
                    self.optical_object={}
                    self.optical_color={}
                    self.optical_connected={}
                    self.inertial_axis_tolerance=const(float(str(settings.settings.get('inertial_axis_tolrance_Gs '))))
                    self.inertial_gyro_tolerance=const(int(str(settings.settings.get('inertial_gyro_tolrance_DEGREES '))))
                    self.inertial_calibrating=False
                    self.inertial_connected=True
                    self.inertial_rotation_history=0
                    self.inertial_roll_history=0
                    self.inertial_pitch_history=0
                    self.inertial_heading_history=0
                    self.inertial_x_axis_history=0
                    self.inertial_y_axis_history=0
                    self.inertial_z_axis_history=0
                    self.rotation_connection={}
                    self.rotation_angle_history={}
                    self.rotation_position_history={}
                    self.distance_tolrance=const(int(str(settings.settings.get('distance_tolrance_MM '))))
                    self.distance_connection={}
                    self.distance_object={}
                    self.distance_history={}

                def motor(self, motor: Motor|None=None) -> None:
                    """Capture for any general smart motor. Enter motor you wish to log as input. (Can take motor groups as well.)"""

                    if motor!=None and motor not in log.MiscMotors:
                        log.MiscMotors.append(motor)

                    for motor_ in log.MiscMotors:
                        self.motor_id=id(motor_)
                        
                        # Setup id to sets if not there.
                        if self.motor_id not in self.setup:
                            self.motor_temp_monitoring[self.motor_id] = 0
                            self.motor_power_monitoring[self.motor_id] = 0
                            self.motor_current_monitoring[self.motor_id] = 0
                            self.motor_disconnected[self.motor_id] = False
                            self.setup[self.motor_id]=True
                            self.motor_name[self.motor_id]=str(motor_)

                        self.motor_temp: int=motor_.temperature(PERCENT)

                        if self.motor_temp==2:
                            if not self.motor_disconnected[self.motor_id]:
                                log.add("EM1", "%s"%(self.motor_name[self.motor_id]))
                                self.motor_disconnected[self.motor_id]=True
                            else:
                                return
                        elif self.motor_temp!=2 and self.motor_disconnected[self.motor_id]:
                            self.motor_disconnected[self.motor_id]=False

                        self.current_motor_power=motor_.power(PowerUnits.WATT)
                        self.current_motor_current:int=motor_.current(CurrentUnits.AMP)
                        
                        # Cheaks for the temps,  power, and cheaks for conecttions of motors(s).
                        if self.motor_temp<=50: 
                            if self.motor_temp_monitoring[self.motor_id]>0:
                                log.add("DM0", "Motor %s, Temp %s"%(self.motor_name[self.motor_id], self.motor_temp))
                                self.motor_temp_monitoring[self.motor_id]=0
                        elif self.motor_temp>70: 
                            if (self.motor_temp_monitoring[self.motor_id]==0 or self.motor_temp_monitoring[self.motor_id]==2):
                                log.add("EM0", "Motor %s, Temp %s"%(self.motor_name[self.motor_id], self.motor_temp))
                                self.motor_temp_monitoring[self.motor_id]=1  
                        elif self.motor_temp>50: 
                            if self.motor_temp_monitoring[self.motor_id]==0:
                                log.add("WM0", "Motor %s, Temp %s"%(self.motor_name[self.motor_id], self.motor_temp))
                                self.motor_temp_monitoring[self.motor_id]=2
                        
                        if self.current_motor_power<=12: 
                            if self.motor_power_monitoring[self.motor_id]>0:
                                log.add("DM1", "Motor %s, Power %s"%(self.motor_name[self.motor_id], str(self.current_motor_power)))
                                self.motor_power_monitoring[self.motor_id]=0
                        elif self.current_motor_power>20: 
                            if (self.motor_power_monitoring[self.motor_id]==0 or self.motor_power_monitoring[self.motor_id]==2):
                                log.add("EM2", "Motor %s, Power %s"%(self.motor_name[self.motor_id], str(self.current_motor_power)))
                                self.motor_power_monitoring[self.motor_id]=1
                        elif self.current_motor_power>12: 
                            if self.motor_power_monitoring[self.motor_id]==0:
                                log.add("WM1", "Motor %s, Power %s"%(self.motor_name[self.motor_id], str(self.current_motor_power)))
                                self.motor_power_monitoring[self.motor_id]=2

                        if self.current_motor_current<=15: 
                            if self.motor_current_monitoring[self.motor_id]>0:
                                log.add("DM2", "Motor %s, Current %1.1f"%(self.motor_name[self.motor_id], self.current_motor_current))
                                self.motor_current_monitoring[self.motor_id]=0
                        elif self.current_motor_current>20: 
                            if (self.motor_current_monitoring[self.motor_id]==0 or self.motor_current_monitoring[self.motor_id]==2):
                                log.add("EM3", "Motor %s, Current %1.1f"%(self.motor_name[self.motor_id], self.current_motor_current))
                                self.motor_current_monitoring[self.motor_id]=1
                        elif self.current_motor_current>15:
                            if self.motor_current_monitoring[self.motor_id]==0:
                                log.add("WM2", "Motor %s, Current %1.1f"%(self.motor_name[self.motor_id], self.current_motor_current))
                                self.motor_current_monitoring[self.motor_id]=2
                
                def optical(self, opticalsensor: Optical) -> None:
                    """Capture for an optical sensor. Enter optical sensor to Capture."""

                    self.optical_id=id(opticalsensor)

                    if  self.optical_id not in self.optical_connected:
                        self.optical_connected[self.optical_id]=True

                    if  self.optical_id not in self.optical_object:
                        self.optical_object[self.optical_id]=False

                    if  self.optical_id not in self.optical_color:
                        self.optical_color[self.optical_id]=0
                    
                    if opticalsensor.installed() and not self.optical_connected[self.optical_id]:
                        
                        log.add("DO3", str(opticalsensor))
                        self.optical_connected[self.optical_id]=True
                    elif not opticalsensor.installed() and self.optical_connected[self.optical_id]:

                        log.add("EO0", str(opticalsensor))
                        self.optical_connected[self.optical_id]=False

                    if opticalsensor.is_near_object():

                        if not self.optical_object[self.optical_id]:

                            log.add("DO1", str(opticalsensor))
                            self.optical_object[self.optical_id]=True

                        if not (self.optical_color[self.optical_id] >= opticalsensor.hue() - log.tolrance and self.optical_color[self.optical_id] <= opticalsensor.hue() + log.tolrance):

                            log.add("DO0", str(opticalsensor.hue()) + ", Sensor " + str(opticalsensor))
                            self.optical_color[self.optical_id]=opticalsensor.hue()
                            
                    elif not opticalsensor.is_near_object() and self.optical_object[self.optical_id]:

                        log.add("DO2", str(opticalsensor))
                        self.optical_object[self.optical_id]=False
                        self.optical_color[self.optical_id]=0

                def inertial(self, inertialsensor: Inertial) -> None:
                    """Capture for inertal sensor. Enter inertial sensor to log."""
        
                    if inertialsensor.installed():

                        self.heading=inertialsensor.heading()
                        self.rotation_values:int=inertialsensor.rotation()
                        self.acceleration_y=inertialsensor.acceleration(AxisType.YAXIS)
                        self.acceleration_z=inertialsensor.acceleration(AxisType.ZAXIS)
                        self.acceleration_x=inertialsensor.acceleration(AxisType.XAXIS)
                        self.pitch=inertialsensor.orientation(OrientationType.PITCH, DEGREES)
                        self.roll=inertialsensor.orientation(OrientationType.ROLL, DEGREES)


                        if not self.inertial_connected:

                            log.add("DI7", "")
                            self.inertial_connected=True

                        if inertialsensor.is_calibrating() and not self.inertial_calibrating:

                            log.add("DI2", "")
                            self.inertial_calibrating=True
                        elif not inertialsensor.is_calibrating() and self.inertial_calibrating:

                            log.add("DI3", "")
                            self.inertial_calibrating=False

                        if not (self.inertial_rotation_history >= self.rotation_values - self.inertial_gyro_tolerance and self.inertial_rotation_history <= self.rotation_values + self.inertial_gyro_tolerance):

                            log.add("DI0", int(self.rotation_values))
                            self.inertial_rotation_history= self.rotation_values

                        if not (self.inertial_roll_history >= self.roll - self.inertial_gyro_tolerance and self.inertial_roll_history <= self.roll + self.inertial_gyro_tolerance):

                            log.add("DI9", int(self.roll))
                            self.inertial_roll_history= self.roll

                        if not (self.inertial_pitch_history >= self.pitch - self.inertial_gyro_tolerance and self.inertial_pitch_history <= self.pitch + self.inertial_gyro_tolerance):

                            log.add("DI8", int(self.pitch))
                            self.inertial_pitch_history= self.pitch
                        
                        if not (self.inertial_heading_history >= self.heading - self.inertial_gyro_tolerance and self.inertial_heading_history <= self.heading + self.inertial_gyro_tolerance):

                            log.add("DI1", int(self.heading))
                            self.inertial_heading_history= self.heading
                        
                        if not (self.inertial_x_axis_history >= self.acceleration_x - self.inertial_axis_tolerance and self.inertial_x_axis_history <= self.acceleration_x + self.inertial_axis_tolerance):

                            log.add("DI4", round(self.acceleration_x, 2))
                            self.inertial_x_axis_history= self.acceleration_x
                        
                        if not (self.inertial_y_axis_history >= self.acceleration_y - self.inertial_axis_tolerance and self.inertial_y_axis_history <= self.acceleration_y + self.inertial_axis_tolerance):

                            log.add("DI5", round(self.acceleration_y, 2))
                            self.inertial_y_axis_history= self.acceleration_y

                        if not (self.inertial_z_axis_history >= self.acceleration_z - self.inertial_axis_tolerance and self.inertial_z_axis_history <= self.acceleration_z + self.inertial_axis_tolerance):
                                
                            log.add("DI6", round(self.acceleration_z, 2))
                            self.inertial_z_axis_history= self.acceleration_z
                            
                    elif self.inertial_connected:

                        log.add("EI0", "")
                        self.inertial_connected=False

                def distance(self, distancesensor: Distance) -> None:
                    distance_id=id(distancesensor)

                    if distance_id not in self.distance_connection:
                        self.distance_connection[distance_id]=True
                    if distance_id not in self.distance_object:
                        self.distance_object[distance_id]=False
                    if distance_id not in self.distance_history:
                        self.distance_history[distance_id]=0

                    if distancesensor.installed() and not self.distance_connection[distance_id]:
                        log.add("DDS3", str(distancesensor))
                        self.distance_connection[distance_id]=True
                    elif not distancesensor.installed() and self.distance_connection[distance_id]:
                        log.add("EDS0", str(distancesensor))
                        self.distance_connection[distance_id]=False

                    if distancesensor.is_object_detected():
                        if not self.distance_object[distance_id]:

                            log.add("DDS0", distancesensor)
                            self.distance_object[distance_id]=True

                        if not (self.distance_history[distance_id] >= distancesensor.object_distance() - self.distance_tolrance and self.distance_history[distance_id] <= distancesensor.object_distance() + self.distance_tolrance):
                            log.add("DDS1", str(distancesensor.object_distance()) + ", Sensor " + str(distancesensor))
                            self.distance_history[distance_id]=distancesensor.object_distance()
                    elif not distancesensor.is_object_detected() and self.distance_object[distance_id]:
                        log.add("DDS4", distancesensor)
                        self.distance_object[distance_id]=False

                def rotation(self, rotationsensor: Rotation) -> None:
                    """Capture for a rotation sensor. Enter rotation sensor to log"""

                    rotaion_id=id(rotationsensor)

                    if rotaion_id not in self.rotation_angle_history:
                        self.rotation_angle_history[rotaion_id]=0
                    if rotaion_id not in self.rotation_position_history:
                        self.rotation_position_history[rotaion_id]=0
                    if rotaion_id not in self.rotation_connection:
                        self.rotation_connection[rotaion_id]=True
                    
                    if rotationsensor.installed() and self.rotation_connection[rotaion_id]==False:

                        log.add("DR0", str(rotationsensor))
                        self.rotation_connection[rotaion_id]=True
                    elif not rotationsensor.installed() and self.rotation_connection[rotaion_id]==True:

                        log.add("ER0", str(rotationsensor))
                        self.rotation_connection[rotaion_id]=False
                    
                    if not (self.rotation_angle_history[rotaion_id] >= rotationsensor.angle() - log.tolrance and self.rotation_angle_history[rotaion_id] <= rotationsensor.angle() + log.tolrance):

                        log.add("DR1", str(rotationsensor.angle()) + ", Sensor " + str(rotationsensor))
                        self.rotation_angle_history[rotaion_id]= rotationsensor.angle()
                    
                    if not (self.rotation_position_history[rotaion_id] >= rotationsensor.position() - log.tolrance and self.rotation_position_history[rotaion_id] <= rotationsensor.position() + log.tolrance):

                        log.add("DR2", str(rotationsensor.position()) + ", Sensor " + str(rotationsensor))
                        self.rotation_position_history[rotaion_id]= rotationsensor.position()
                
            class Threewire:
                def __init__(self):
                    self.digital_value={}
                    self.analog_value={}
                
                def digitalinput(self, input: DigitalIn) -> None:
                    """Capture for digital input. Enter Digital input to log."""

                    input_id=id(input)

                    if input_id not in self.digital_value:
                        self.digital_value[input_id]=0

                    if input.value()==1 and self.digital_value[input_id]==0:
                        log.add("DDI0", "")
                        self.digital_value[input_id]=1
                    elif input.value()==0 and self.digital_value[input_id]==1:
                        log.add("DDI1", "")
                        self.digital_value[input_id]=0

                def analog(self, input: AnalogIn) -> None:
                    """Capture for analog inputs. Enter analog input to log."""

                    input_id=id(input)

                    if input_id not in self.analog_value:
                        self.analog_value[input_id]=0

                    if not (self.analog_value[input_id] >= input.value() - log.tolrance and self.analog_value[input_id] <= input.value() + log.tolrance):
                        log.add("DAI0", input.value())
                        self.analog_value[input_id]=input.value()
                
                def bumper(self, bumpersensor: Bumper) -> None:
                    """Capture for bumper switchs. Enter bumper to log."""

                    bumper_id=id(bumpersensor)

                    if bumper_id not in self.digital_value:
                        self.digital_value[bumper_id]=0

                    if bumpersensor.value()==1 and self.digital_value[bumper_id]==0:
                        log.add("DBS0", "")
                        self.digital_value[bumper_id]=1
                    elif bumpersensor.value()==0 and self.digital_value[bumper_id]==1:
                        log.add("DBS1", "")
                        self.digital_value[bumper_id]=0

                def limit(self, limitsensor: Limit) -> None:
                    """Capture for limit switchs. Enter bumper to log."""

                    limit_id=id(limitsensor)

                    if limit_id not in self.digital_value:
                        self.digital_value[limit_id]=0

                    if limitsensor.value()==1 and self.digital_value[limit_id]==0:
                        log.add("DLS0", "")
                        self.digital_value[limit_id]=1
                    elif limitsensor.value()==0 and self.digital_value[limit_id]==1:
                        log.add("DLS1", "")
                        self.digital_value[limit_id]=0

                def potentiometer(self, sensor: PotentiometerV2) -> None:
                    """Capture for potentiometer inputs. Enter potentiometer input to log."""

                    sensor_id=id(sensor)

                    if sensor_id not in self.analog_value:
                        self.analog_value[sensor_id]=0

                    if not (self.analog_value[sensor_id] >= sensor.angle() - log.tolrance and self.analog_value[sensor_id] <= sensor.angle() + log.tolrance):
                        log.add("DP0", sensor.angle())
                        self.analog_value[sensor_id]=sensor.angle()

                def pwm(self, input: Pwm) -> None:
                    """
                    Capture for pwm inputs. 
                    Enter pwm input to log.

                    Args:
                    input= Pwm()
                    """

                    input_id=id(input)

                    if input_id not in self.analog_value:
                        self.analog_value[input_id]=0

                    if not (self.analog_value[input_id] >= input.value() - log.tolrance and self.analog_value[input_id] <= input.value() + log.tolrance):
                        log.add("DPW0", input.value())
                        self.analog_value[input_id]=input.value()    

            def __init__(self):
                self.smartport=self.Smartport()
                self.threewire=self.Threewire()
                self.system=self.System()
                # set for variables id.
                self.variables={}
                self.valueid=0
                self.battery_voltage=0
                self.battery_current=0
                self.battery_capacity=0
                self.battery_watts=0
                self.battery_voltage_monitoring=0
                self.battery_current_monitoring=0
                self.battery_capacity_monitoring=0
                self.battery_watts_monitoring=0
                self.ctrl_name={}
                self.watts:int=0
                self.axis={}
                self.button_objs={}
                self.button_names = [
                    "A", "B", "X", "Y",
                    "UP", "DOWN", "LEFT", "RIGHT",
                    "L1", "L2", "R1", "R2",
                ]
                self.button_values = {}
                self.drivetrain_temp_monitoring={}
                self.drivetrain_power_monitoring={}
                self.drivetrain_current_monitoring={}
                self.drivetrain_disconnected={}
                self.drivetrain_setup={}
                self.drivetrain_name={}

            def drivetrain(self, leftmotors: list[Motor], rightmotors: list[Motor]) -> None:
                """
                Initialize the drivetrain with left and right motors.

                Args:
                    leftmotors: List of left motors.
                    rightmotors: List of right motors.
                """
                self.LeftMotors = leftmotors
                self.RightMotors = rightmotors

                leftside=MotorGroup(self.LeftMotors)
                rightside=MotorGroup(self.RightMotors)

                motor_list = [leftside, rightside]

                for motor_ in motor_list:
                    self.motor_id=id(motor_)
                        
                    # Setup id to sets if not there.
                    if self.motor_id not in self.drivetrain_setup:
                        self.drivetrain_temp_monitoring[self.motor_id] = 0
                        self.drivetrain_power_monitoring[self.motor_id] = 0
                        self.drivetrain_current_monitoring[self.motor_id] = 0
                        self.drivetrain_disconnected[self.motor_id] = False
                        self.drivetrain_setup[self.motor_id]=True
                        self.drivetrain_name[self.motor_id]=str(motor_)

                    self.motor_temp: int=motor_.temperature(TemperatureUnits.CELSIUS)

                    if self.motor_temp==2:
                        if not self.drivetrain_disconnected[self.motor_id]:
                            log.add("EM1", "%s"%(self.drivetrain_name[self.motor_id]))
                            self.drivetrain_disconnected[self.motor_id]=True
                        else:
                            return
                    elif self.motor_temp!=2 and self.drivetrain_disconnected[self.motor_id]:
                        self.drivetrain_disconnected[self.motor_id]=False

                    self.current_motor_power=motor_.power(PowerUnits.WATT)
                    self.current_motor_current:int=motor_.current(CurrentUnits.AMP)
                    
                    # Cheaks for the temps,  power, and cheaks for conecttions of motors(s).
                    if self.motor_temp<=50: 
                        if self.drivetrain_temp_monitoring[self.motor_id]>0:
                            log.add("DM0", "Motor %s, Temp %s"%(self.drivetrain_name[self.motor_id], self.motor_temp))
                            self.drivetrain_temp_monitoring[self.motor_id]=0
                    elif self.motor_temp>70: 
                        if (self.drivetrain_temp_monitoring[self.motor_id]==0 or self.drivetrain_temp_monitoring[self.motor_id]==2):
                            log.add("EM0", "Motor %s, Temp %s"%(self.drivetrain_name[self.motor_id], self.motor_temp))
                            self.drivetrain_temp_monitoring[self.motor_id]=1  
                    elif self.motor_temp>50: 
                        if self.drivetrain_temp_monitoring[self.motor_id]==0:
                            log.add("WM0", "Motor %s, Temp %s"%(self.drivetrain_name[self.motor_id], self.motor_temp))
                            self.drivetrain_temp_monitoring[self.motor_id]=2
                    
                    if self.current_motor_power<=12: 
                        if self.drivetrain_power_monitoring[self.motor_id]>0:
                            log.add("DM1", "Motor %s, Power %s"%(self.drivetrain_name[self.motor_id], str(self.current_motor_power)))
                            self.drivetrain_power_monitoring[self.motor_id]=0
                    elif self.current_motor_power>20: 
                        if (self.drivetrain_power_monitoring[self.motor_id]==0 or self.drivetrain_power_monitoring[self.motor_id]==2):
                            log.add("EM2", "Motor %s, Power %s"%(self.drivetrain_name[self.motor_id], str(self.current_motor_power)))
                            self.drivetrain_power_monitoring[self.motor_id]=1
                    elif self.current_motor_power>12: 
                        if self.drivetrain_power_monitoring[self.motor_id]==0:
                            log.add("WM1", "Motor %s, Power %s"%(self.drivetrain_name[self.motor_id], str(self.current_motor_power)))
                            self.drivetrain_power_monitoring[self.motor_id]=2

                    if self.current_motor_current<=15: 
                        if self.drivetrain_current_monitoring[self.motor_id]>0:
                            log.add("DM2", "Motor %s, Current %1.1f"%(self.drivetrain_name[self.motor_id], self.current_motor_current))
                            self.drivetrain_current_monitoring[self.motor_id]=0
                    elif self.current_motor_current>20: 
                        if (self.drivetrain_current_monitoring[self.motor_id]==0 or self.drivetrain_current_monitoring[self.motor_id]==2):
                            log.add("EM3", "Motor %s, Current %1.1f"%(self.drivetrain_name[self.motor_id], self.current_motor_current))
                            self.drivetrain_current_monitoring[self.motor_id]=1
                    elif self.current_motor_current>15:
                        if self.drivetrain_current_monitoring[self.motor_id]==0:
                            log.add("WM2", "Motor %s, Current %1.1f"%(self.drivetrain_name[self.motor_id], self.current_motor_current))
                            self.drivetrain_current_monitoring[self.motor_id]=2



            def battery(self) -> None:
                """
                Capture for the brains battery. 
                
                Args:
                None
                """
                
                self.battery_voltage=brain.battery.voltage(VOLT)
                self.battery_current=brain.battery.current(CurrentUnits.AMP)
                self.battery_capacity=brain.battery.capacity()
                self.battery_watts=self.battery_current * self.battery_voltage

                # Battery monitoring for voltage, capacity, and current.
                if self.battery_voltage>=12:
                    if self.battery_voltage_monitoring==1 or self.battery_voltage_monitoring==2:
                        log.add("DB0", "%s"%(self.battery_voltage))
                        self.battery_voltage_monitoring=0
                elif self.battery_voltage<12:
                    if self.battery_voltage_monitoring==0 or self.battery_voltage_monitoring==1:
                        log.add("WB0", "%s"%(self.battery_voltage))
                        self.battery_voltage_monitoring=2
                elif self.battery_voltage<11:
                    if self.battery_voltage_monitoring==0 or self.battery_voltage_monitoring==2:
                        log.add("EB0", "%s"%(self.battery_voltage))
                        self.battery_voltage_monitoring=1

                if self.battery_capacity>=50:
                    if self.battery_capacity_monitoring!=self.battery_capacity:
                        log.add("DB3", "%s"%(self.battery_capacity))
                        self.battery_capacity_monitoring=self.battery_capacity
                elif self.battery_capacity<50:
                    if self.battery_capacity_monitoring!=self.battery_capacity:
                        log.add("WB1", "%s"%(self.battery_capacity))
                        self.battery_capacity_monitoring=self.battery_capacity
                elif self.battery_capacity<25:
                    if self.battery_capacity_monitoring!=self.battery_capacity:
                        log.add("EB1", "%s"%(self.battery_capacity))
                        self.battery_capacity_monitoring=self.battery_capacity

                if self.battery_current<=10:
                    if self.battery_current_monitoring==1 or self.battery_current_monitoring==2:
                        log.add("DB1", "%s"%(self.battery_current))
                        self.battery_current_monitoring=0
                elif self.battery_current>10:
                    if self.battery_current_monitoring==0 or self.battery_current_monitoring==1:
                        log.add("WB2", "%s"%(self.battery_current))
                        self.battery_current_monitoring=2
                elif self.battery_current>15:
                    if self.battery_current_monitoring==0 or self.battery_current_monitoring==2:
                        log.add("EB2", "%s"%(self.battery_current))
                        self.battery_current_monitoring=1
                
                if self.battery_watts<=150:
                    if self.battery_watts_monitoring==1 or self.battery_watts_monitoring==2:
                        log.add("DB2", "%s"%(self.battery_watts))
                        self.battery_watts_monitoring=0
                elif self.battery_watts>150:
                    if self.battery_watts_monitoring==0 or self.battery_watts_monitoring==1:
                        log.add("WB3", "%s"%(self.battery_watts))
                        self.battery_watts_monitoring=2
                elif self.battery_watts>200:
                    if self.battery_watts_monitoring==0 or self.battery_watts_monitoring==3:
                        log.add("EB3", "%s"%(self.battery_watts))
                        self.battery_watts_monitoring=1

            def controller(self, controller: Controller) -> None:
                """
                Capture for the controllers. 
                Enter controller you wish to log. 
                
                Args:
                controller= Controller()
                """

                controllerid=const(id(controller))

                if controllerid not in self.button_values:
                    self.button_values[controllerid]=[True,True,True,True,True,True,True,True,True,True,True,True]

                    self.button_objs[controllerid] = [
                    controller.buttonA,
                    controller.buttonB,
                    controller.buttonX,
                    controller.buttonY,
                    controller.buttonUp,
                    controller.buttonDown,
                    controller.buttonLeft,
                    controller.buttonRight,
                    controller.buttonL1,
                    controller.buttonL2,
                    controller.buttonR1,
                    controller.buttonR2,
                    ]

                    self.ctrl_name[controllerid]=str(controller)

                
                if controllerid not in self.axis:
                    self.axis[controllerid]=[0, 0, 0, 0]

                self.c_axis1 = controller.axis1.position()
                self.c_axis2 = controller.axis2.position()
                self.c_axis3 = controller.axis3.position()
                self.c_axis4 = controller.axis4.position()

                if self.c_axis1 != 0 and not (self.axis[controllerid][0] >= self.c_axis1 - log.tolrance and self.axis[controllerid][0] <= self.c_axis1 + log.tolrance):
                    log.add(log.controller_axis, "%s, Axis1, %d"%(self.ctrl_name[controllerid], self.c_axis1))
                    self.axis[controllerid][0] = self.c_axis1
                elif self.c_axis1 == 0 and self.axis[controllerid][0] != 0:
                    log.add(log.controller_axis, "%s, Axis1, 0"%(self.ctrl_name[controllerid]))
                    self.axis[controllerid][0] = 0

                if self.c_axis2 != 0 and not (self.axis[controllerid][1] >= self.c_axis2 - log.tolrance and self.axis[controllerid][1] <= self.c_axis2 + log.tolrance):
                    log.add(log.controller_axis, "%s, Axis2, %d"%(self.ctrl_name[controllerid], self.c_axis2))
                    self.axis[controllerid][1] = self.c_axis2
                elif self.c_axis2 == 0 and self.axis[controllerid][1] != 0:
                    log.add(log.controller_axis, "%s, Axis2, 0"%(self.ctrl_name[controllerid]))
                    self.axis[controllerid][1] = 0

                if self.c_axis3 != 0 and not (self.axis[controllerid][2] >= self.c_axis3 - log.tolrance and self.axis[controllerid][2] <= self.c_axis3 + log.tolrance):
                    log.add(log.controller_axis, "%s, Axis3, %d"%(self.ctrl_name[controllerid], self.c_axis3))
                    self.axis[controllerid][2] = self.c_axis3
                elif self.c_axis3 == 0 and self.axis[controllerid][2] != 0:
                    log.add(log.controller_axis, "%s, Axis3, 0"%(self.ctrl_name[controllerid]))
                    self.axis[controllerid][2] = 0

                if self.c_axis4 != 0 and not (self.axis[controllerid][3] >= self.c_axis4 - log.tolrance and self.axis[controllerid][3] <= self.c_axis4 + log.tolrance):
                    log.add(log.controller_axis, "%s, Axis4, %d"%(self.ctrl_name[controllerid], self.c_axis4))
                    self.axis[controllerid][3] = self.c_axis4
                elif self.c_axis4 == 0 and self.axis[controllerid][3] != 0:
                    log.add(log.controller_axis, "%s, Axis4, 0"%(self.ctrl_name[controllerid]))
                    self.axis[controllerid][3] = 0

                # Button logging for controller.

                for i in range(12):
                    if self.button_objs[controllerid][i].pressing():
                        if self.button_values[controllerid][i]:
                            log.add(log.controller_button, "%s, %s, Pressed"%(self.ctrl_name[controllerid], self.button_names[i]))
                            self.button_values[controllerid][i] = False
                    else:
                        if not self.button_values[controllerid][i]:
                            log.add(log.controller_button, "%s, %s, Released"%(self.ctrl_name[controllerid], self.button_names[i]))
                            self.button_values[controllerid][i] = True

            def variable(self, name: str, value: Any) -> None:
                """
                Capture for int, float, and bool variables. 
                Enter name of variable in a string and then the variable you wish to log.
                
                Args:
                name= String
                value= Int, Boolean, Float
                """

                self.valueid=const(str(name))

                # Adds id if not in set.
                if self.valueid not in self.variables:

                    if type(value)==bool:
                        self.variables[self.valueid]=False
                    elif type(value)==int:
                        self.variables[self.valueid]=0
                    elif type(value)==float:
                        self.variables[self.valueid]=0.0
                    elif type(value)==str:
                        self.variables[self.valueid]=""
                    elif type(value)==list:
                        self.variables[self.valueid]=[]
                    elif type(value)==dict:
                        self.variables[self.valueid]={}
                    elif type(value)==tuple:
                        self.variables[self.valueid]=()
                    else:
                        self.variables[self.valueid]="Unknown Type: %s"%(type(value))

                if value != self.variables[self.valueid]:
                    log.add(log.variable, "%s, Val %s"%(name, value))
                    self.variables[self.valueid] = value

    class Log:
        """Main object for the CLEAR import. \n To start logging use the "logstart()" function in this object to do the main logging if you need help with its inputs use help() over the "logstart()" function."""
                    
        def __init__(self):
            if not brain.sdcard.exists("loghistory.txt"):
                brain.sdcard.savefile("loghistory.txt")
            self.MiscMotors: list[Motor]=[]
            self.LeftMotors: list[Motor]=[]
            self.RightMotors: list[Motor]=[]
            self.capture=Capture()
            self.archive=Archive()
            self._index:int=0
            self.adding:bool=True  # Used to pause logging.
            self.format:str="utf-8"  # General format for all files in the code.
            self._cache:bytearray=bytearray()
            self.brainscreen:bool=False  # Used to see if need to print to brain screen.
            self.tolrance:int=3  # tolerance for controller stick diffrence when not recording and for general tolrance for sensors.
            self.printing:bool=True
            self.logging:bool=True
            self.buffer=bytearray(20480)
            self._bufferSize=0
            self._buffer_offset=0
            self._last_write=0
            self.robot_active=False
            self.memory_change=const("DSM0")
            self.module_change=const("DSP0")
            self.aton_start=const("DSC0")
            self.controller_axis=const("DC1")
            self.controller_button=const("DC0")
            self.variable=const("DV0")

            brain.sdcard.savefile("Logstart.txt")  # Clears Logstart file for refresh of instructions in it.

            # Predefined Log Codes dictionary
            self.codes:dict={
            """Main dictionary for CLEAR"""
                    "EB0": b"<Batt ERR: Very Low Volt>",
                    "EB1": b"<Batt ERR: Very Low Batt>",
                    "EB2": b"<Batt ERR: Very High Curr>",
                    "EB3": b"<Batt ERR: Very High Watt>",
                    "EB4": b"<Batt ERR: Very High Temp>",
                    "WB0": b"<Batt WARN: Low Volt>",
                    "WB1": b"<Batt WARN: Low Batt>",
                    "WB2": b"<Batt WARN: High Curr>",
                    "WB3": b"<Batt WARN: High Watt>",
                    "WB4": b"<Batt WARN: High Temp>",
                    "DB0": b"<Batt DATA: Volt Normal>",
                    "DB1": b"<Batt DATA: Curr Normal>",
                    "DB2": b"<Batt DATA: Watt Normal>",
                    "DB3": b"<Batt DATA: Capacity Changed>",
                    "DB4": b"<Batt DATA: Temp Normal>",
                    "DA1": b"<Aton DATA: Recording Stopped>",
                    "DA2": b"<Aton DATA: Recording Saved>",
                    "DA3": b"<Aton DATA: Recording Loaded>",
                    "DS0": b"<System DATA: Init setup Done>",
                    "DS1": b"<System DATA: Archived Log>",
                    "DS2": b"<System DATA: Index Log History Done>",
                    "DS3": b"<System DATA: Archived Recording>",
                    "DS5": b"<System DATA: Recalled Recording>",
                    "EM0": b"<Motor ERR: Very Hot>",
                    "EM1": b"<Motor ERR: Disconnected>",
                    "EM2": b"<Motor ERR: Very High Power>",
                    "EM3": b"<Motor ERR: Very High Current>",
                    "WM0": b"<Motor WARN: Hot>",
                    "WM1": b"<Motor WARN: High Power>",
                    "WM2": b"<Motor WARN: High Current>",
                    "DM0": b"<Motor DATA: Temp Normal>",
                    "DM1": b"<Motor DATA: Power Normal>",
                    "DM2": b"<Motor DATA: Current Back To Normal>",
                    "DV0": b"<Variable DATA: Changed>",
                    "DC0": b"<Cont DATA: Button Changed>",
                    "DC1": b"<Cont DATA: Axis Changed>",
                    "DPW0": b"<Pwm DATA: Value Changed>",
                    "DP0": b"<Pot DATA: Value Changed>",
                    "DLS1": b"<Limit DATA: Released>",
                    "DLS0": b"<Limit DATA: Pressed>",
                    "DO3": b"<Optical DATA: Installed>",
                    "EO0": b"<Optical ERROR: Disconnected>",
                    "DO1": b"<Optical DATA: Detected Object>",
                    "DO0": b"<Optical DATA: Color Changed. Color:",
                    "DO2": b"<Optical DATA: Lost Object>",
                    "DI7": b"<Inertial DATA: Installed>",
                    "DI2": b"<Inertial DATA: Calibrating>",
                    "DI3": b"<Inertial DATA: Calibration Complete>",
                    "DI0": b"<Inertial DATA: Rotation Changed>",
                    "DI9": b"<Inertial DATA: Roll Changed>",
                    "DI8": b"<Inertial DATA: Pitch Changed>",
                    "DI1": b"<Inertial DATA: Heading Changed>",
                    "DI4": b"<Inertial DATA: X Axis Changed>",
                    "DI5": b"<Inertial DATA: Y Axis Changed>",
                    "DI6": b"<Inertial DATA: Z Axis Changed>",
                    "EI0": b"<Inertial ERROR: Disconnected>",
                    "DDS3": b"<Distance DATA: Installed>",
                    "EDS0": b"<Distance ERROR: Disconnected>",
                    "DDS0": b"<Distance DATA: Detected Object>",
                    "DDS1": b"<Distance DATA: Distance Changed>",
                    "DDS4": b"<Distance DATA: Lost Object>",
                    "DR0": b"<Rotation DATA: Installed>",
                    "ER0": b"<Rotation ERROR: Disconnected>",
                    "DR1": b"<Rotation DATA: Angle Changed>",
                    "DR2": b"<Rotation DATA: Position Changed>",
                    "DDI0": b"<Digital DATA: High>",
                    "DDI1": b"<Digital DATA: Low>",
                    "DAI0": b"<Analog DATA: Changed Value>",
                    "DBS0": b"<Bumper DATA: Pressed>",
                    "DBS1": b"<Bumper DATA: Released>",
                    "DSM0": b"<Mem DATA: Memory Useage Changed> ",
                    "DSP0": b"<Sys DATA: New module(s) added>",
                    "DSC0": b"<Comp DATA: Aton Started>",
                    "DSC1": b"<Comp DATA: Driver Started>",
                    "DSC2": b"<Comp DATA: Connected>",
                    "DSC3": b"<Comp DATA: Field Connected>",
                    "DSC4": b"<Comp DATA: Field Disconnected>",
                    "DSC5": b"<Comp DATA: Disconnected>"
                    }
            
            # Setting up Log Files if they dont exist and setting index.
            log_number=0
            if not brain.sdcard.exists("Log.csv"):
                brain.sdcard.savefile("Log.csv", bytearray("log Start: \n", self.format))
            else:
                with open("Log.csv", 'rb') as log_file:
                    chunk_size=10240
                    while True:
                        chunk=log_file.read(chunk_size)
                        if not chunk:
                            break

                        log_number+=chunk.count(b'\n')

                self._index= log_number - 1

        def append_log(self) -> None:
            """
            Appends to log file.

            Args:
            entry= String
            """

            if ((log_time.time()-self._last_write>3000 and self._bufferSize !=0) and self.robot_active) or self._buffer_offset > 2200: 
                brain.sdcard.appendfile("Log.csv", self.buffer[0:self._buffer_offset])
                print("saved.")
                self._buffer_offset=0
                self._last_write=log_time.time()
                self._bufferSize=0
                self.robot_active=False
        
        def brain_read(self) -> None:
            """
            Prints to brain screen.

            Args:
            entry= String
            """

            if brain.screen.row()>=20:
                brain.screen.clear_screen()
                brain.screen.set_cursor(1,1)

            brain.screen.print(self.entry.decode(log.format))
            brain.screen.new_line()
        
        def add(self, add_code: str, add_details: Any) -> None:
            """
            Main funtion for Log.

            Takes code and the added details gets runtime and index. 
            Then, prints it, puts them in Log or cache, and see if it needs to print to brain screen. 
            Enter code then the details you want as a string.

            Args:
            add_code= String
            add_details= Any
            """

            if not self.adding:
                self._cache.extend(const(b"%d [%d], %b, %s\n" % (self._index, log_time.time(), self.codes.get(add_code), add_details)))
                return 
            else:
                if self._cache:
                    self.entry=self._cache
                    if self.printing:
                        print(self._cache.decode(self.format))
                    if self.logging:
                        brain.sdcard.appendfile("Log.csv", self._cache)
                    if self.brainscreen:
                        self.brain_read()
                    self._cache=bytearray()
                    return
                
            self.entry=const(b"%d [%d ms], %s, %s\n" %(self._index, log_time.time(), self.codes.get(add_code), add_details))
            self._bufferSize=len(self.entry)

            pack_into("=%ds"%(self._bufferSize), self.buffer, self._buffer_offset, self.entry)
            self._buffer_offset+=self._bufferSize

            if self.logging:
                self.append_log()

            if self.brainscreen:  # Checks if pinting to brainscreen is enabled.
                self.brain_read()
            
            if self.printing:
                print(self.entry)
            
            if log_link.is_linked():
                log_link.send(self.entry.decode(log.format))

            self._index += 1
            
        def add_codes(self, code_add: str, Decoded_text: str) -> None:
            """
            Adds codes to the codes dictionary. 
            Enter new code key, then the full string.

            Args:
            code_add= String
            Decoded_text= String
            """

            self.codes.update({code_add : "%s"%(Decoded_text)})

        def remove_codes(self, code_remove: str) -> None:
            """
            Removes codes from dictionary.
            Enter code key to remove.

            Args:
            code_remove= String
            """

            if code_remove in self.codes:
                self.codes.pop(code_remove)
            else:
                print("Code Not Found In Log Codes")

        def edit_codes(self, code_edit: str, new_decoded_text: str) -> None:
            """
            Edits existing codes in dictionary. 
            Enter code key and then new full string
            
            Args:
            code_edit= String
            new_decoded_text= String
            """

            if code_edit in self.codes:
                self.codes.update({code_edit : "%s"%(new_decoded_text)})

        # Clearing the log file
        def clear(self) -> None:
            """
            Clears the Log.csv file.
            
            Args:
            None
            """

            brain.sdcard.savefile("Log.csv", bytearray("Log Start: \n", self.format))
        
        def add_logstart(self, funtion) -> None:
            """
            Used in logstart. 
            Enter the funtion for variables like so "log.capture.variable()" then in the inner parentheses add the name of variable as a string and the variable. 
            Can also be used for any capture funtion in CLEAR.

            Args:
            funtion= Funtion for object Log
            """

            brain.sdcard.appendfile("Logstart.txt" , bytearray(funtion + "\n", self.format))
            
        def auto_start(self):
            """
            The main way to CLEAR.
            All that is need is to call it.

            Args:
            None
            """

            self.archive.index_history()
            if brain.sdcard.filesize("Log.csv") > 100000:
                self.archive.log()
            
            self.format:str=const(str(settings.settings.get('format_used ')))
            self.tolrance:int=const(int(str(settings.settings.get('default_tolrance '))))
            wait_time_logging:int=const(int(str(settings.settings.get('logging_loop_wait '))))

            if "True" in str(settings.settings.get('gc_use ')):
                gc_use:bool=const(True)
            else:
                gc_use:bool=const(False)

            if "True" in str(settings.settings.get('log_battery ')):
                log_battery:bool=const(True)
            else:
                log_battery:bool=const(False)

            if "True" in str(settings.settings.get('log_memory ')):
                log_memory:bool=const(True)
            else:
                log_memory:bool=const(False)

            if "True" in str(settings.settings.get('log_modules ')):
                log_modules:bool=const(True)
            else:
                log_modules:bool=const(False)

            if "True" in str(settings.settings.get('print_read ')):
                self.printing:bool=const(True)
            else:
                self.printing:bool=const(False)

            if "True" in str(settings.settings.get('sdcard_read ')):
                self.logging:bool=const(True)
            else:
                self.logging:bool=const(False)

            if "True" in str(settings.settings.get('brain_read ')):
                self.brainscreen:bool=const(True)
            else:
                self.brainscreen:bool=const(False)

            if self.brainscreen:
                brain.screen.set_font(FontType.MONO12)

            # Logs system start.
            self.add("DS0", "")
            
            if "True" in str(settings.settings.get('auto_do_variables ')):
                auto_do_variables:bool=const(True)
            else:
                auto_do_variables:bool=const(False)

            if "True" in str(settings.settings.get('auto_do_control ')):
                auto_do_control:bool=const(True)
            else:
                auto_do_control:bool=const(False)

            if "True" in str(settings.settings.get('auto_do_three_wire ')):
                auto_do_three_wire:bool=const(True)
            else:
                auto_do_three_wire:bool=const(False)

            if "True" in str(settings.settings.get('auto_do_smart_port ')):
                auto_do_smart_port:bool=const(True)
            else:
                auto_do_smart_port:bool=const(False)

            if "True" in str(settings.settings.get('auto_do_motors ')):
                auto_do_motors:bool=const(True)
            else:
                auto_do_motors:bool=const(False)

            if "True" in str(settings.settings.get('auto_do_controller ')):
                auto_do_controller:bool=const(True)
            else:
                auto_do_controller:bool=const(False)

            controllers: List[Controller]=[]

            globallogging=const(dir())

            #print(globallogging)

            for item in globallogging:
                
                try:
                    item_type=str(type(eval(item)))
                except NameError:
                    continue

                if  (item_type == "<class 'int'>" or item_type == "<class 'bool'>" or item_type == "<class 'float'>" or item_type == "<class 'str'>" or item_type == "<class 'list'>" or item_type == "<class 'dict'>" or item_type == "<class 'tuple'>") and auto_do_variables:
                    log.add_logstart("log.capture.variable('%s', %s)"%(item, item.replace("'", "")))
                elif item_type == "<class 'motor'>" and auto_do_motors:
                    if "Left" in str(item) or "left" in str(item):
                        self.LeftMotors+=[eval(item)]
                    elif "Right" in str(item) or "right" in str(item):
                        self.RightMotors+=[eval(item)]
                    else:
                        self.MiscMotors+=[eval(item)]
                elif item_type == "<class 'controller'>" and auto_do_controller:
                    controllers+=[eval(item)]
                elif item_type == "<class 'inertial'>" and auto_do_smart_port:
                    log.add_logstart("log.capture.smartport.inertial(%s)"%(item.replace("'", "")))
                elif item_type == "<class 'optical'>" and auto_do_smart_port:
                    log.add_logstart("log.capture.smartport.optical(%s)"%(item.replace("'", "")))
                elif item_type == "<class 'rotation'>" and auto_do_smart_port:
                    log.add_logstart("log.capture.smartport.rotation(%s)"%(item.replace("'", "")))
                elif item_type == "<class 'distance'>" and auto_do_smart_port:
                    log.add_logstart("log.capture.smartport.distance(%s)"%(item.replace("'", "")))
                elif item_type == "<class 'triport_bumper'>" and auto_do_three_wire:
                    log.add_logstart("log.capture.threewire.bumper(%s)"%(item.replace("'", "")))
                elif item_type == "<class 'triport_limit'>" and auto_do_three_wire:
                    log.add_logstart("log.capture.threewire.limit(%s)"%(item.replace("'", "")))
                elif item_type == "<class 'triport_digitalin'>" and auto_do_three_wire:
                    log.add_logstart("log.capture.threewire.digitalinput(%s)"%(item.replace("'", "")))
                elif item_type == "<class 'triport_potv2'>" and auto_do_three_wire:
                    log.add_logstart("log.capture.threewire.potentiometer(%s)"%(item.replace("'", "")))
                elif item_type == "<class 'triport_analog'>" and auto_do_three_wire:
                    log.add_logstart("log.capture.threewire.analog(%s)"%(item.replace("'", "")))
                elif item_type == "<class 'comp'>" and auto_do_control:
                    log.add_logstart("log.capture.system.control(%s)"%(item.replace("'", "")))
                    

            del auto_do_variables, auto_do_three_wire, auto_do_control, auto_do_motors, auto_do_smart_port

            # print("Logstart: ")
            # print(brain.sdcard.loadfile("Logstart.txt").decode(self.format))

            _exec=exec
            lwait=wait
            capture_memory=self.capture.system.memoryuse
            capture_modules=self.capture.system.modules
            capture_battery=log.capture.battery
            timer=log_time.time
            gc_collect=collect
            motorcapture=log.capture.smartport.motor
            drivetraincapture=log.capture.drivetrain
            controllercapture=log.capture.controller
            log_check=log.append_log

            # Loads extra funtions from file.
            try:
                addedfuntion=brain.sdcard.loadfile("Logstart.txt").decode(self.format)
                added_bytes=compile(addedfuntion, '<string>' ,'exec', 0,  False, 2)
                added_bytes_used=const(True)
            except AttributeError:
                addedfuntion=""
                added_bytes=compile("", '<string>' ,'exec', 0,  True, 2)
                added_bytes_used=const(False)
            
            del addedfuntion
            
            gc_collect()

            while True:
                for _ in range(200):
                    start:int=const(timer())

                    if not self.robot_active:
                        if not auto_do_controller:
                            self.robot_active=True
                        
                        if len(controllers)>0:
                            if  (controllers[0].axis2.position() !=0 or controllers[0].axis3.position() !=0):
                                self.robot_active=True
                        else:
                            self.robot_active=True
                    else:
                        log_check()

                    if controllers:
                        for controller in controllers:
                            controllercapture(controller)

                    if added_bytes_used:
                        _exec(added_bytes)
                    
                    if self.MiscMotors:
                        motorcapture()

                    if self.LeftMotors and self.RightMotors:
                        drivetraincapture(self.LeftMotors, self.RightMotors)

                    if log_memory:
                        capture_memory()

                    if log_modules:
                        capture_modules()
                    
                    if log_battery:
                        capture_battery()
                    
                    #print(timer()-start)

                    print(mem_alloc())

                    lwait(wait_time_logging - (timer() - start))

                if gc_use:
                    gc_collect()
        
        def start(self) -> Thread:
            cla=Thread(self.auto_start)
            return cla
        
        def in_function_auto_start(self):
            globallogging=const(dir())

            #print(globallogging)

            for item in globallogging:
                try:
                    item_type=str(type(eval(item)))
                except NameError:
                    continue

                if  (item_type == "<class 'int'>" or item_type == "<class 'bool'>" or item_type == "<class 'float'>" or item_type == "<class 'str'>" or item_type == "<class 'list'>" or item_type == "<class 'dict'>" or item_type == "<class 'tuple'>"):
                    log.add_logstart("log.capture.variable('%s', %s)"%(item, item.replace("'", "")))

            # Loads extra funtions from file.
            try:
                addedfuntion=brain.sdcard.loadfile("Logstart.txt").decode(self.format)
                added_bytes=compile(addedfuntion, '<string>' ,'exec', 0,  False, 2)
                added_bytes_used=const(True)
            except AttributeError:
                addedfuntion=""
                added_bytes=compile("", '<string>' ,'exec', 0,  True, 2)
                added_bytes_used=const(False)
            
            del addedfuntion

            while True:
                start:int=const(log_time.time())

                if added_bytes_used:
                    exec(added_bytes)
                
                #print(timer()-start)

                # mem_info(1)
                print(mem_alloc())

                wait(100 - (log_time.time() - start))

        
        def in_function_start(self) -> Thread:
            cla=Thread(self.in_function_auto_start)
            return cla

        def __enter__(self) -> Thread:
            self.Thread=log.in_function_start()
            return self.Thread

        def __exit__(self) -> None:
            self.Thread.stop()     
    
    class Archive:
            """
            Main object for archiving files made by CLEAR.
            """
            
            def log(self) -> None:
                """
                Archives the Log.txt file.
                
                Args:
                None
                """

                speed=log_time.time()
                log.adding=False

                reversecodes={value: key for key, value in log.codes.items()}
                
                with open("Log.csv", "rb") as file:
                    chunk_buffer=bytearray(20480)
                    archivelist=bytearray(20480)
                    archivelist_offset=0
                    
                    while True:
                        chunk=file.readinto(chunk_buffer)

                        if not chunk:
                            break
                        
                        loglist=chunk_buffer.decode(log.format).split("\n")
                        incomplete=bytearray()
                        for i in range(len(loglist)):
                            logline= loglist[i].split(",")

                            if archivelist_offset > 20000:
                                brain.sdcard.appendfile("loghistory.txt", archivelist[0:archivelist_offset])
                                archivelist_offset=0
                            
                            if len(logline)>=3:
                                logstring=logline[1].strip()
                                numbers=logline[0].split(" ")
                                try:
                                    hexindex="{:#x}".format(int(numbers[0]))
                                    hextime="{:#x}".format(int(numbers[1].replace("[", "")))
                                except ValueError:
                                    incomplete.extend(loglist[i].encode(log.format))
                                    continue
                                entry=b"%s %s %s%s\n"%(hexindex, hextime, reversecodes.get(logstring, logstring), str(logline[2:len(logline)]).replace("'", "").replace("[", "").replace("]", ""))
                                bufferSize=len(entry)
                                pack_into("=%ds"%(bufferSize), archivelist, archivelist_offset, entry)
                                archivelist_offset+=bufferSize
                            else:
                                incomplete.extend(loglist[i].encode(log.format))
                                if incomplete.decode(log.format).count('>') != 0:
                                    logline=incomplete.decode(log.format).split(",")
                                    if len(logline)>=3:
                                        logstring=logline[1].strip()
                                        numbers=logline[0].split(" ")
                                        try:
                                            hexindex="{:#x}".format(int(numbers[0]))
                                            hextime="{:#x}".format(int(numbers[1].replace(" ms]", "").replace("[]", "")))
                                        except ValueError:
                                            incomplete.extend(loglist[i].encode(log.format))
                                            continue
                                        entry=b"%s %s %s%s\n"%(hexindex, hextime, reversecodes.get(logstring, logstring), str(logline[2:len(logline)-1]).replace("'", "").replace("[", "").replace("]", ""))
                                        bufferSize=len(entry)
                                        pack_into("=%ds"%(bufferSize), archivelist, archivelist_offset, entry)
                                        archivelist_offset+=bufferSize
                                        incomplete=bytearray()
                                
                            del logline
                        brain.sdcard.appendfile("loghistory.txt", archivelist[0:archivelist_offset])
                        archivelist_offset=0
                        print("done loop")
                log.clear()
                log.adding=True

                collect()
                log.add("DS1", str(log_time.time() - speed) + " MSEC")
                del speed

            def recording(self, recordingname: str) -> None:
                """
                Archives recording file. 
                Enter full name of file.

                Args:
                recordingname= String
                """

                print("Archiving...")
                filename=str(recordingname).replace(".txt", "_archived.txt")
                brain.sdcard.savefile(filename)
                with open(recordingname, 'rb') as recording:
                    chunk_buffer=bytearray(10240)
                    buffer=bytearray()
                    while True:
                        chunk=recording.readinto(chunk_buffer)
                        if not chunk:
                            break
                        
                        list=chunk_buffer.split(b"\n")
                        for line in list:
                            prelist=line.split(b' ')
                            buffer.extend(b"%b %b %b %b \n" %(prelist[2], prelist[9], prelist[10], prelist[11]))
                        brain.sdcard.appendfile(filename, buffer)
    
                brain.sdcard.savefile(recordingname)
                log.add("DS3", recordingname)

            def index_history(self) -> None:
                """
                Gets lines of loghistory puts number in file.
                
                Args:
                None
                """

                speed=log_time.time()
                index=0
                chunk=0
                
                with open("loghistory.txt", 'rb') as file:
                    chunk_buffer=bytearray(10240)
                    while True:
                        chunk = file.readinto(chunk_buffer)
                        if not chunk:
                            break
                        index += bytes(chunk_buffer[0: chunk]).count(b'\n')
                log._index+=index
                log.add("DS2", str(log_time.time() - speed) + " MSEC")
                del speed, index

            def recall_log(self) -> None:
                """
                Unarchives The log.

                Args:
                None
                """

                filename=("logrecalled.csv")
                print("recalling...")
                with open("loghistory.txt", 'r') as file:
                    for line in file:
                        prelist=line.split(' ')
                        if len(prelist) >= 4:
                            detailslist=[item + " " for item in prelist[3 : len(prelist)-1]]
                        details="".join(detailslist)
                        brain.sdcard.appendfile(filename, bytearray("%d [%s ms], %s, %s\n"%(int(prelist[0], 16),int(prelist[1], 16) , log.codes.get(prelist[2], prelist[2]), details), log.format))
                print("Recall done.")
            
            def recall_recording(self, name: str) -> None:
                """
                Restores recording file to an uncompressed state. 
                Enter full name of the archived file.

                Args:
                name=String, full name of file
                """

                recording=brain.sdcard.loadfile(name).decode(log.format).split('\n')
                filename=name.replace("_archived.txt", ".txt")
                brain.sdcard.savefile(filename)
                for item in recording:
                    prelist=item.split(' ')
                    if "Moved" in item:
                        brain.sdcard.appendfile(filename, bytearray("[',', '0', %s ':Controller', 'DATA:', 'Axis', 'Changed.', 'Axis:', '', %s %s 'Moved', '0', 'Degrees', ''] \n"%(prelist[0], prelist[1], prelist[2]), log.format))
                    elif "Pressed" in item or "Released" in item:
                        brain.sdcard.appendfile(filename, bytearray("[',', '0', %s ':Controller', 'DATA:', 'Button', 'Changed.', 'Button:', '', %s %s %s ''] \n"%(prelist[0], prelist[1], prelist[2], prelist[3]), log.format))
                log.add("DS5", name)

    class Settings:
        """Used to congigure the log in a more permenet way using the Sd card"""

        def __init__(self):
            self.changes=""
            self.settings={}
            self.default_settings_dictonary={
                "brain_read": False,
                "print_read": True,
                "sdcard_read": True,
                "gc_use": True,
                "archive_log": True,
                "archive_recordings": True,
                "log_memory": False,
                "log_modules": True,
                "log_battery": True,
                "logging_loop_wait": 200,
                "recording_loop_wait": -1,
                "format_used": "utf-8",
                "auto_do_motors": True,
                "auto_do_variables": True,
                "auto_do_control": True,
                "auto_do_three_wire": True,
                "auto_do_smart_port": True,
                "auto_do_controller": True,
                "default_tolrance": 3,
                "memory_tolrance_KB": 100,
                "distance_tolrance_MM": 100,
                "inertial_gyro_tolrance_DEGREES": 5,
                "inertial_axis_tolrance_Gs": 0.5,
            }
            
            if brain.sdcard.is_inserted() and not brain.sdcard.exists("settings.txt"):
                setting=""
                for value, key in self.default_settings_dictonary.items():
                    setting+="%s : %s \n"%(value, key)
                brain.sdcard.savefile("settings.txt", bytearray(setting, "utf-8"))
                self.settings_text=brain.sdcard.loadfile("settings.txt").decode("utf-8").split("\n")
                for line in self.settings_text:
                    dict_stuff=line.split(":")

                    if len(dict_stuff) >= 2:
                        self.settings[dict_stuff[0]]=dict_stuff[1]
                
            else:
                self.settings_text=brain.sdcard.loadfile("settings.txt").decode("utf-8").split("\n")
                for line in self.settings_text:
                    dict_stuff=line.split(":")

                    if len(dict_stuff) >= 2:
                        self.settings[dict_stuff[0]]=dict_stuff[1]
    
    settings=Settings()
    log=Log()

except Exception as e:
    import uio
    if not _errorcreated:
        brain.sdcard.savefile("Error.txt")
    exeption_string=uio.StringIO(500)
    sys.print_exception(e, exeption_string)  # type: ignore
    print(exeption_string.getvalue())
    brain.sdcard.appendfile("Error.txt", bytearray(b"<System ERROR> Python Runtime Error. \n %s "%(exeption_string.getvalue())))
    
    del exeption_string, uio