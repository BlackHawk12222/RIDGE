# ---------------------------------------------------------------------------- #
#                                                                              #
# 	Module:       CLA.py                                                       #
# 	Author:       Micah Bow                                                    #
# 	Created:      1/27/2026, 12:42 PM                                          #
#   Last Edited:  7/20/2026, 12:00 AM                                          #
# 	Description:  Capture, Logging, Archiving.                                 #
#                                                                              #
# ---------------------------------------------------------------------------- #

_errorcreated=False
from vex import *
import sys
brain=Brain()

try:
    from gc import collect, mem_alloc  # type: ignore 
    from ustruct import pack_into
    from micropython import const  # type: ignore
    from . import ControllerInterface as CInterface
    
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
                    self.memory_tolrance: int=int(str(settings.settings.get('memory_tolrance_KB ')))
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
                    self.setup: dict[int, bool] = {}
                    self.motor_id=0
                    self.optical_object={}
                    self.optical_color={}
                    self.optical_connected={}
                    self.inertial_axis_tolerance=float(str(settings.settings.get('inertial_axis_tolrance_Gs ')))
                    self.inertial_gyro_tolerance=int(str(settings.settings.get('inertial_gyro_tolrance_DEGREES ')))
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
                    self.distance_tolrance=int(str(settings.settings.get('distance_tolrance_MM ')))
                    self.distance_connection={}
                    self.distance_object={}
                    self.distance_history={}

                def setup_motor(self, motor_) -> None:
                    """One-time setup of a motor's monitoring entries (done at start, not every loop)."""
                    motor_id=id(motor_)
                    if motor_id in self.setup:
                        return
                    self.motor_temp_monitoring[motor_id] = 0
                    self.motor_power_monitoring[motor_id] = 0
                    self.motor_current_monitoring[motor_id] = 0
                    self.motor_disconnected[motor_id] = False
                    self.setup[motor_id]=True

                def motor(self, motor: Motor|None=None, motorName: str|None=None) -> None:
                    """Capture for any general smart motor. Enter motor you wish to log as input. (Can take motor groups as well.)"""

                    if motor!=None and motor not in log.MiscMotors:
                        log.MiscMotors.append(motor)
                        self.setup_motor(motor)
                    
                    if motorName!=None and motorName not in log.MiscMotorsName:
                        log.MiscMotorsName.append(motorName)

                    for i in range(len(log.MiscMotors)):
                        motor_=log.MiscMotors[i]
                        self.motor_id=id(motor_)

                        self.motor_temp: int=motor_.temperature(PERCENT)

                        if self.motor_temp==2:
                            if not self.motor_disconnected[self.motor_id]:
                                log.add("EM1", "%s"%(log.MiscMotorsName[i]))
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
                                log.add("DM0", "Motor %s, Temp %s"%(log.MiscMotorsName[i], self.motor_temp))
                                self.motor_temp_monitoring[self.motor_id]=0
                        elif self.motor_temp>70: 
                            if (self.motor_temp_monitoring[self.motor_id]==0 or self.motor_temp_monitoring[self.motor_id]==2):
                                log.add("EM0", "Motor %s, Temp %s"%(log.MiscMotorsName[i], self.motor_temp))
                                self.motor_temp_monitoring[self.motor_id]=1  
                        elif self.motor_temp>50: 
                            if self.motor_temp_monitoring[self.motor_id]==0:
                                log.add("WM0", "Motor %s, Temp %s"%(log.MiscMotorsName[i], self.motor_temp))
                                self.motor_temp_monitoring[self.motor_id]=2
                        
                        if self.current_motor_power<=12: 
                            if self.motor_power_monitoring[self.motor_id]>0:
                                log.add("DM1", "Motor %s, Power %s"%(log.MiscMotorsName[i], str(self.current_motor_power)))
                                self.motor_power_monitoring[self.motor_id]=0
                        elif self.current_motor_power>20: 
                            if (self.motor_power_monitoring[self.motor_id]==0 or self.motor_power_monitoring[self.motor_id]==2):
                                log.add("EM2", "Motor %s, Power %s"%(log.MiscMotorsName[i], str(self.current_motor_power)))
                                self.motor_power_monitoring[self.motor_id]=1
                        elif self.current_motor_power>12: 
                            if self.motor_power_monitoring[self.motor_id]==0:
                                log.add("WM1", "Motor %s, Power %s"%(log.MiscMotorsName[i], str(self.current_motor_power)))
                                self.motor_power_monitoring[self.motor_id]=2

                        if self.current_motor_current<=15: 
                            if self.motor_current_monitoring[self.motor_id]>0:
                                log.add("DM2", "Motor %s, Current %1.1f"%(log.MiscMotorsName[i], self.current_motor_current))
                                self.motor_current_monitoring[self.motor_id]=0
                        elif self.current_motor_current>20: 
                            if (self.motor_current_monitoring[self.motor_id]==0 or self.motor_current_monitoring[self.motor_id]==2):
                                log.add("EM3", "Motor %s, Current %1.1f"%(log.MiscMotorsName[i], self.current_motor_current))
                                self.motor_current_monitoring[self.motor_id]=1
                        elif self.current_motor_current>15:
                            if self.motor_current_monitoring[self.motor_id]==0:
                                log.add("WM2", "Motor %s, Current %1.1f"%(log.MiscMotorsName[i], self.current_motor_current))
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

                def rotation(self, RotationSensor: Rotation, RotaionSensorName: str) -> None:
                    """Capture for a rotation sensor. Enter rotation sensor to log"""

                    rotaion_id=id(RotationSensor)

                    if rotaion_id not in self.rotation_angle_history:
                        self.rotation_angle_history[rotaion_id]=0
                    if rotaion_id not in self.rotation_position_history:
                        self.rotation_position_history[rotaion_id]=0
                    if rotaion_id not in self.rotation_connection:
                        self.rotation_connection[rotaion_id]=True
                    
                    if RotationSensor.installed() and self.rotation_connection[rotaion_id]==False:

                        log.add("DR0", RotaionSensorName)
                        self.rotation_connection[rotaion_id]=True
                    elif not RotationSensor.installed() and self.rotation_connection[rotaion_id]==True:

                        log.add("ER0", RotaionSensorName)
                        self.rotation_connection[rotaion_id]=False
                    
                    if not (self.rotation_angle_history[rotaion_id] >= RotationSensor.angle() - log.tolrance and self.rotation_angle_history[rotaion_id] <= RotationSensor.angle() + log.tolrance):

                        log.add("DR1", "%d, Sensor %s"%(RotationSensor.angle(), RotaionSensorName))
                        self.rotation_angle_history[rotaion_id]= RotationSensor.angle()
                    
                    if not (self.rotation_position_history[rotaion_id] >= RotationSensor.position() - log.tolrance and self.rotation_position_history[rotaion_id] <= RotationSensor.position() + log.tolrance):

                        log.add("DR2", "%d, Sensor %s"%(RotationSensor.position(), RotaionSensorName))
                        self.rotation_position_history[rotaion_id]= RotationSensor.position()
                
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
                self.rightside=Motor(Ports.PORT21)
                self.leftside=Motor(Ports.PORT21)
                self.DrivetrainSetup=False
                self.drivetrain_list=[self.leftside, self.rightside]

            def setup_drivetrain(self) -> None:
                """One-time setup of both drivetrain sides' monitoring entries (done at start, not every loop)."""
                self.drivetrain_list=[self.leftside, self.rightside]
                names=("LeftSide", "RightSide")
                for i in range(2):
                    motor_id=id(self.drivetrain_list[i])
                    if motor_id in self.drivetrain_setup:
                        continue
                    self.drivetrain_temp_monitoring[motor_id] = 0
                    self.drivetrain_power_monitoring[motor_id] = 0
                    self.drivetrain_current_monitoring[motor_id] = 0
                    self.drivetrain_disconnected[motor_id] = False
                    self.drivetrain_name[motor_id]=names[i]
                    self.drivetrain_setup[motor_id]=True
                self.DrivetrainSetup=True

            def drivetrain(self, leftmotors: list[Motor]|None=None, rightmotors: list[Motor]|None=None) -> None:
                """
                Initialize the drivetrain with left and right motors.

                Args:
                    leftmotors: List of left motors.
                    rightmotors: List of right motors.
                """

                if (leftmotors !=None and rightmotors!=None and log.LeftMotors != leftmotors) or not self.DrivetrainSetup:
                    if leftmotors and rightmotors:
                        log.LeftMotors = leftmotors
                        log.RightMotors = rightmotors
                        self.leftside=log.LeftMotors[0]
                        self.rightside=log.RightMotors[0]
                    self.setup_drivetrain()

                motor_list = self.drivetrain_list

                for i in range(2):
                    motor_=motor_list[i]
                    self.motor_id=id(motor_)

                    self.motor_temp: int=motor_.temperature(PERCENT)  # type: ignore

                    if self.motor_temp==2:
                        if not self.drivetrain_disconnected[self.motor_id]:
                            log.add("EM1", "%s"%(self.drivetrain_name[self.motor_id]))
                            self.drivetrain_disconnected[self.motor_id]=True
                        else:
                            return
                    elif self.motor_temp!=2 and self.drivetrain_disconnected[self.motor_id]:
                        self.drivetrain_disconnected[self.motor_id]=False

                    self.current_motor_power=motor_.power(PowerUnits.WATT)  # type: ignore
                    self.current_motor_current:int=motor_.current(CurrentUnits.AMP)  # type: ignore
                    
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

                controllerid=id(controller)

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
                name = String;
                value = Any;
                """

                if value != self.variables[name]:
                    log.add(log.variable, "%s, Val %s"%(name, value))
                    self.variables[name] = value

            def const_variable(self, name: str, value: Any) -> None:
                log.add(log.variable, "%s, Val %s"%(name, value))

    class Log:
        """Main object for the CLEAR import. \n To start logging use the "logstart()" function in this object to do the main logging if you need help with its inputs use help() over the "logstart()" function."""
                    
        def __init__(self):
            if not brain.sdcard.exists("ACPv1/Data/loghistory.txt"):
                brain.sdcard.savefile("ACPv1/Data/loghistory.txt", bytearray(b"0                   \n"))

            self.MiscMotors: list[Motor]=[]
            self.MiscMotorsName: list[str]=[]
            self.LeftMotors: list[Motor]=[]
            self.RightMotors: list[Motor]=[]
            self.capture=Capture()
            self.archive=Archive()
            self._index:int=0
            self.format:str="utf-8"  # General format for all files in the code.
            self._cache:bytearray=bytearray()
            self.brainscreen:bool=False  # Used to see if need to print to brain screen.
            self.controller_interface=False  # Bool for ControllerInterface.py use.
            self.tolrance:int=3  # Tolerance for controller stick diffrence when not recording and for general tolrance for sensors.
            self.printing:bool=True
            self.logging:bool=True
            self.buffer=bytearray(20480)
            self._bufferSize=0
            self._buffer_offset=0
            self._last_write=0
            self.robot_active=False
            self.memory_change="DSM0"
            self.module_change="DSP0"
            self.aton_start="DSC0"
            self.controller_axis="DC1"
            self.controller_button="DC0"
            self.variable="DV0"
            self.VariablesAdded=[]

            brain.sdcard.savefile("ACPv1/Config/Logstart.txt")  # Clears Logstart file for refresh of instructions in it.

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
                    "DV0": b"<Var DATA: Changed>",
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
                    "DI7": b"<Inert DATA: Installed>",
                    "DI2": b"<Inert DATA: Calibrating>",
                    "DI3": b"<Inert DATA: Calibration Complete>",
                    "DI0": b"<Inert DATA: Rotation Changed>",
                    "DI9": b"<Inert DATA: Roll Changed>",
                    "DI8": b"<Inert DATA: Pitch Changed>",
                    "DI1": b"<Inert DATA: Heading Changed>",
                    "DI4": b"<Inert DATA: X Axis Changed>",
                    "DI5": b"<Inert DATA: Y Axis Changed>",
                    "DI6": b"<Inert DATA: Z Axis Changed>",
                    "EI0": b"<Inert ERROR: Disconnected>",
                    "DDS3": b"<Dist DATA: Installed>",
                    "EDS0": b"<Dist ERROR: Disconnected>",
                    "DDS0": b"<Dist DATA: Detected Object>",
                    "DDS1": b"<Dist DATA: Distance Changed>",
                    "DDS4": b"<Dist DATA: Lost Object>",
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

            self.reversecodes: dict= {value: key for key, value in self.codes.items()}
            
            # Setting up Log Files if they dont exist and setting index.
            log_number=0
            self._log_lines=1  # Lines in Log.csv including header; used by Archive.log().
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
                self._log_lines=log_number

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

            entry: list[bytes]=[self.entry]

            if len(self.entry) >= 81:
                chunk_size=81
                entry=[self.entry[i : i + chunk_size] for i in range(0, len(self.entry), chunk_size)]

            if b"DATA" in self.entry:
                brain.screen.set_pen_color(Color.GREEN)
            elif b"WARN" in self.entry:
                brain.screen.set_pen_color(Color.YELLOW)
            elif b"ERR" in self.entry:
                brain.screen.set_pen_color(Color.RED)
            else:
                brain.screen.set_pen_color(Color.WHITE)

            for i in range(len(entry)):
                brain.screen.print(entry[i].decode(log.format))
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
                
            timestamp: int = log_time.time()
            self.entry: bytes = b"%d [%d ms], %s, %s\n" %(self._index, timestamp, self.codes.get(add_code), add_details)
            self._bufferSize: int = len(self.entry)

            pack_into(b"=%ds"%(self._bufferSize), self.buffer, self._buffer_offset, self.entry)
            self._buffer_offset += self._bufferSize

            if self.logging:
                self.append_log()

            if self.brainscreen:  # Checks if pinting to brainscreen is enabled.
                self.brain_read()
            
            if self.printing:
                print(self.entry)
            
            # if log_link.is_linked():
            #     log_link.send(self.entry.decode(log.format))

            if self.controller_interface and (b"ERR" in self.entry or b"WARN" in self.entry):
                CInterface.add_to_buffer("%s %s" %(add_code, add_details))

            self._index += 1
            
        def add_codes(self, code_add: str, Decoded_text: str) -> None:
            """
            Adds codes to the codes dictionary. 
            Enter new code key, then the full string.

            Args:
            code_add= String
            Decoded_text= String
            """

            self.codes.update({code_add : b"%s"%(Decoded_text)})
            self.reversecodes.update({b"%s"%(Decoded_text) : code_add})

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

            brain.sdcard.appendfile("ACPv1/Config/Logstart.txt" , bytearray(funtion + "\n", self.format))
            
        def auto_start(self):
            """
            The main way to CLEAR.
            All that is need is to call it.

            Args:
            None
            """

            start:int=log_time.time()

            self.format:str=str(settings.settings.get('format_used ')).strip()
            self.tolrance:int=int(str(settings.settings.get('default_tolrance ')))
            wait_time_logging:int=int(str(settings.settings.get('logging_loop_wait ')))

            if "True" in str(settings.settings.get('gc_use ')):
                gc_use:bool=True
            else:
                gc_use:bool=False

            if "True" in str(settings.settings.get('log_battery ')):
                log_battery:bool=True
            else:
                log_battery:bool=False

            if "True" in str(settings.settings.get('log_memory ')):
                log_memory:bool=True
            else:
                log_memory:bool=False

            if "True" in str(settings.settings.get('log_modules ')):
                log_modules:bool=True
            else:
                log_modules:bool=False

            if "True" in str(settings.settings.get('print_read ')):
                self.printing:bool=True
            else:
                self.printing:bool=False

            if "True" in str(settings.settings.get('sdcard_read ')):
                self.logging:bool=True
            else:
                self.logging:bool=False

            if "True" in str(settings.settings.get('brain_read ')):
                self.brainscreen:bool=True
            else:
                self.brainscreen:bool=False

            if "True" in str(settings.settings.get('controller_interface ')):
                self.controller_interface=True
            else:
                self.controller_interface=False
            
            if self.brainscreen:
                brain.screen.set_font(FontType.MONO12)
            
            if "True" in str(settings.settings.get('auto_do_variables ')):
                auto_do_variables:bool=True
            else:
                auto_do_variables:bool=False

            if "True" in str(settings.settings.get('auto_do_control ')):
                auto_do_control:bool=True
            else:
                auto_do_control:bool=False

            if "True" in str(settings.settings.get('auto_do_three_wire ')):
                auto_do_three_wire:bool=True
            else:
                auto_do_three_wire:bool=False

            if "True" in str(settings.settings.get('auto_do_smart_port ')):
                auto_do_smart_port:bool=True
            else:
                auto_do_smart_port:bool=False

            if "True" in str(settings.settings.get('auto_do_motors ')):
                auto_do_motors:bool=True
            else:
                auto_do_motors:bool=False

            if "True" in str(settings.settings.get('auto_do_controller ')):
                auto_do_controller:bool=True
            else:
                auto_do_controller:bool=False

            self.archive.index_history()
            if brain.sdcard.filesize("Log.csv") > 100000:
                self.archive.log()

            self.controllers: List[Controller]=[]

            scope=dir()

            log_add_logstart = self.add_logstart

            def set_var(name: str, obj):
                if name.isupper() or ((name.endswith("__") and name.startswith("__"))):  # Check if variable is all caps and is a dunder variable eg. "TOTAL" or "__path__" repectively.
                    log.capture.const_variable(name, obj)
                    return
                
                log_add_logstart("_cv('%s', %s)"%(name, name.replace("'", "")))
                self.VariablesAdded.append(name)

            def set_motor(name, obj):
                if "Left" in name or "left" in name:
                    self.LeftMotors+=[obj]
                elif "Right" in name or "right" in name:
                    self.RightMotors+=[obj]
                else:
                    self.MiscMotors+=[obj]
                    self.MiscMotorsName+=[name]

            def set_controller(name, obj):
                self.controllers+=[obj]

                if ("Interface", "interface") in name:
                    Thread(lambda: CInterface.start(obj))

            def set_inertial(name, obj):
                log_add_logstart("_spi(%s)"%(name.replace("'", "")))

            def set_optical(name, obj):
                log_add_logstart("_spo(%s)"%(name.replace("'", "")))

            def set_rotation(name, obj):
                log_add_logstart("_spr(%s, '%s')"%(name.replace("'", ""), name))

            def set_distance(name, obj):
                log_add_logstart("_spd(%s)"%(name.replace("'", "")))

            def set_bumper(name, obj):
                log_add_logstart("_twb(%s)"%(name.replace("'", "")))

            def set_limit(name, obj):
                log_add_logstart("_twl(%s)"%(name.replace("'", "")))

            def set_digital_in(name, obj):
                log_add_logstart("_twd(%s)"%(name.replace("'", "")))

            def set_potv2(name, obj):
                log_add_logstart("_twp(%s)"%(name.replace("'", "")))

            def set_analog(name, obj):
                log_add_logstart("_twa(%s)"%(name.replace("'", "")))

            def set_comp(name, obj):
                log_add_logstart("_sc(%s)"%(name.replace("'", "")))

            ConfigDict = {
                "<class 'int'>" : set_var if auto_do_variables else None,
                "<class 'bool'>" : set_var if auto_do_variables else None,
                "<class 'float'>" : set_var if auto_do_variables else None,
                "<class 'str'>" : set_var if auto_do_variables else None,
                "<class 'list'>" : set_var if auto_do_variables else None,
                "<class 'dict'>" : set_var if auto_do_variables else None,
                "<class 'tuple'>" : set_var if auto_do_variables else None,
                "<class 'motor'>" : set_motor if auto_do_motors else None,
                "<class 'controller'>" : set_controller if auto_do_controller else None,
                "<class 'inertial'>" : set_inertial if auto_do_smart_port else None,
                "<class 'optical'>" : set_optical if auto_do_smart_port else None,
                "<class 'rotation'>" : set_rotation if auto_do_smart_port else None,
                "<class 'distance'>" : set_distance if auto_do_smart_port else None,
                "<class 'triport_bumper'>" : set_bumper if auto_do_three_wire else None,
                "<class 'triport_limit'>" : set_limit if auto_do_three_wire else None,
                "<class 'triport_digitalin'>" : set_digital_in if auto_do_three_wire else None,
                "<class 'triport_potv2'>" : set_potv2 if auto_do_three_wire else None,
                "<class 'triport_analog'>" : set_analog if auto_do_three_wire else None,
                "<class 'comp'>" : set_comp if auto_do_control else None  
            }

            for item in scope:
                
                try:
                    eval_item = eval(item)
                except NameError:
                    continue

                
                func = ConfigDict.get(str(type(eval_item)))
                if func != None:
                    func(item, eval_item)

            ModulesChecked=[]

            for Module, ModuleType in sys.modules.items():

                if Module in ModulesChecked:
                    continue

                exec("import %s"%(Module))

                #print(ModuleType)

                ModulesChecked.append(Module)
                try:
                    ModuleLogging=dir(ModuleType)
                except NameError:
                    continue

                for item in ModuleLogging:
                    
                    try:
                        item_type=str(type(eval("%s.%s"%(Module, item))))
                        # print(item, item_type)
                    except NameError:
                        continue
                    except AttributeError:
                        continue

                    if  (item_type == "<class 'int'>" or item_type == "<class 'bool'>" or item_type == "<class 'float'>" or item_type == "<class 'str'>" or item_type == "<class 'list'>" or item_type == "<class 'dict'>" or item_type == "<class 'tuple'>") and auto_do_variables and item not in self.VariablesAdded:
                        log.add_logstart("_cv('%s.%s', %s.%s)"%(Module, item, Module, item.replace("'", "")))
                        self.VariablesAdded.append(item)
                    
            del auto_do_variables, auto_do_three_wire, auto_do_control, auto_do_motors, auto_do_smart_port, ConfigDict, scope, ModulesChecked, log_add_logstart
            del set_var, set_motor, set_controller, set_inertial, set_optical, set_rotation, set_distance, set_bumper, set_limit, set_digital_in, set_potv2, set_analog, set_comp

            # Monitoring entries are set up once here, so the loop never checks for them.
            setup_motor=self.capture.smartport.setup_motor
            for motor_ in self.MiscMotors:
                setup_motor(motor_)
            if self.LeftMotors and self.RightMotors:
                self.capture.leftside=self.LeftMotors[0]
                self.capture.rightside=self.RightMotors[0]
                self.capture.setup_drivetrain()
            del setup_motor

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
            drivetrainuse= True if self.LeftMotors and self.RightMotors else False
            controllers = self.controllers

            # Loads extra funtions from file.
            try:
                addedfuntion=brain.sdcard.loadfile("ACPv1/Config/Logstart.txt").decode(self.format)
                added_bytes=compile(addedfuntion, '<string>' ,'exec', 0, False, 2)
                added_bytes_used=True
            except AttributeError:
                addedfuntion=""
                added_bytes=compile("", '<string>' ,'exec', 0,  True, 2)
                added_bytes_used=False
            
            del addedfuntion

            # Logstart lines run with these capture functions as locals (one dict lookup each,
            # instead of log -> .capture -> .smartport -> .func). Variables still come from the live globals.
            exec_globals=globals()
            cap=log.capture
            exec_locals={
                "_cv": cap.variable,
                "_spi": cap.smartport.inertial,
                "_spo": cap.smartport.optical,
                "_spr": cap.smartport.rotation,
                "_spd": cap.smartport.distance,
                "_twb": cap.threewire.bumper,
                "_twl": cap.threewire.limit,
                "_twd": cap.threewire.digitalinput,
                "_twp": cap.threewire.potentiometer,
                "_twa": cap.threewire.analog,
                "_sc": cap.system.control,
            }
            del cap

            # Logs system start.
            self.add("DS0", "%d MSEC"%(log_time.time()-start))
            
            gc_collect()

            while True:
                for _ in range(2000):
                    start:int=timer()

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
                        _exec(added_bytes, exec_globals, exec_locals)
                    
                    if self.MiscMotors:
                        motorcapture()

                    if drivetrainuse:
                        drivetraincapture(self.LeftMotors, self.RightMotors)

                    if log_memory:
                        capture_memory()

                    if log_modules:
                        capture_modules()
                    
                    if log_battery:
                        capture_battery()

                    lwait(wait_time_logging - (timer() - start))

                if gc_use:
                    gc_collect()
    
    class Archive:
            """
            Main object for archiving files made by CLEAR.
            """
            
            def log(self) -> None:
                """
                Archives the oldest lines of Log.csv into loghistory.txt, keeping the newest 2000.
                No memoryview (stripped from V5 MicroPython 1.13): full chunks are written
                straight from one reused buffer; only the last partial chunk is sliced.
                """
                speed=log_time.time()
                KEEP=2000
                HIST="ACPv1/Data/loghistory.txt"
                TMP="ACPv1/Data/temp.tmp"
                CHUNK=32768
                FLUSH=4096  # Archive entries are joined and written about every 4 KB.

                total=log._log_lines - 1
                to_archive=total - KEEP
                if to_archive <= 0:
                    return

                collect()
                buf=bytearray(CHUNK)

                def copy(src, dst):
                    if buf != None:
                        readinto=src.readinto
                        write=dst.write
                        while True:
                            n=readinto(buf)
                            if not n:
                                break
                            write(buf if n == CHUNK else buf[:n])

                rc={text: code.encode() for text, code in log.reversecodes.items()}
                rc_get=rc.get

                archived=0
                pending=[]
                size=0
                with open("Log.csv", "rb") as f:
                    with open(HIST, "ab") as h:
                        header=f.readline()
                        readline=f.readline
                        write=h.write
                        for _ in range(to_archive):
                            line=readline()
                            if not line:
                                break
                            parts=line.rstrip(b"\r\n").split(b", ")
                            p0=parts[0]
                            ts=int(p0[p0.find(b"[") + 1 : p0.rfind(b" ms")])
                            code=parts[1].strip() if len(parts) > 1 else b""
                            entry=b"%x,%s,%s\n" % (ts, rc_get(code, code), b" ".join(parts[2:]))
                            pending.append(entry)
                            size+=len(entry)
                            if size >= FLUSH:
                                write(b"".join(pending))
                                pending=[]
                                size=0
                            archived+=1
                        if pending:
                            write(b"".join(pending))
                        pending=None

                    with open(TMP, "wb") as t:
                        t.write(header)
                        copy(f, t)

                with open(TMP, "rb") as t:
                    with open("Log.csv", "wb") as f:
                        copy(t, f)

                with open(HIST, "r+b") as h:
                    first=h.readline()
                    new_header=b"%-20d\n" % (int(first) + archived)
                    in_place=len(first) == len(new_header)
                    if in_place:
                        h.seek(0)
                        h.write(new_header)

                if not in_place:
                    with open(HIST, "rb") as h:
                        h.readline()
                        with open(TMP, "wb") as t:
                            t.write(new_header)
                            copy(h, t)
                    with open(TMP, "rb") as t:
                        with open(HIST, "wb") as h:
                            copy(t, h)

                brain.sdcard.savefile(TMP)  # Empty temp.tmp so it does not hold SD space.
                buf=None
                log._log_lines=1 + total - archived
                collect()
                log.add("DS1", str(log_time.time() - speed) + " MSEC")

            def index_history(self) -> None:
                """
                Gets lines of loghistory puts number in file.
                
                Args:
                None
                """

                speed=log_time.time()
                index=0
                
                with open("ACPv1/Data/loghistory.txt", 'r') as file:
                    file.seek(0)
                    index=int(file.readline())

                log._index+=index
                log.add("DS2", str(log_time.time() - speed) + " MSEC")

            def recall_log(self) -> None:
                """
                Unarchives The log into logrecalled.csv. Reads both old ", " and new "," entries.

                Args:
                None
                """

                print("recalling...")
                codes=log.codes
                out=[]
                index=0
                with open("ACPv1/Data/loghistory.txt", "rb") as f, open("logrecalled.csv", "ab") as o:
                    f.readline()  # Skip the count header.
                    for line in f:
                        parts=line.rstrip(b"\r\n").split(b",", 2)
                        if len(parts) < 2:
                            continue
                        code=parts[1].strip().decode()
                        details=parts[2].strip() if len(parts) > 2 else b""
                        out.append(b"%d [%d ms], %s, %s\n" % (index, int(parts[0].strip(), 16), codes.get(code, code.encode()), details))
                        index+=1
                        if len(out) >= 200:
                            o.write(b"".join(out))
                            out=[]
                    if out:
                        o.write(b"".join(out))
                print("Recall done.")

    class Settings:
        """Used to congigure the log in a more permenet way using the Sd card"""

        def __init__(self):
            self.changes=""
            self.settings={}
            self.default_settings_dictonary={
                "brain_read": False,
                "print_read": True,
                "sdcard_read": True,
                "controller_interface": True,
                "gc_use": False,
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
            
            if brain.sdcard.is_inserted() and not brain.sdcard.exists("ACPv1/Config/settings.txt"):
                setting=""
                for value, key in self.default_settings_dictonary.items():
                    setting+="%s : %s \n"%(value, key)
                brain.sdcard.savefile("ACPv1/Config/settings.txt", bytearray(setting, "utf-8"))
                self.settings_text=brain.sdcard.loadfile("ACPv1/Config/settings.txt").decode("utf-8").split("\n")
                for line in self.settings_text:
                    dict_stuff=line.split(":")

                    if len(dict_stuff) >= 2:
                        self.settings[dict_stuff[0]]=dict_stuff[1]
                
            else:
                self.settings_text=brain.sdcard.loadfile("ACPv1/Config/settings.txt").decode("utf-8").split("\n")
                for line in self.settings_text:
                    dict_stuff=line.split(":")

                    if len(dict_stuff) >= 2:
                        self.settings[dict_stuff[0]]=dict_stuff[1]
    
    settings=Settings()
    log=Log()

    class IFCLA:
        def __init__(self):
            self.OldExcVals=[]
        
        def in_function_auto_start(self):
            GlobalLogging=dir()

            for Item in GlobalLogging:
                try:
                    item_type=str(type(eval(Item)))
                except NameError:
                    continue

                if str(Item) not in log.VariablesAdded and  (item_type == "<class 'int'>" or item_type == "<class 'bool'>" or item_type == "<class 'float'>" or item_type == "<class 'str'>" or item_type == "<class 'list'>" or item_type == "<class 'dict'>" or item_type == "<class 'tuple'>"):
                    log.add_logstart("log.capture.variable('%s', %s)"%(Item, Item.replace("'", "")))

            # Loads extra funtions from file.
            try:
                AddedFuntion=brain.sdcard.loadfile("ACPv1/Config/Logstart.txt").decode(log.format)
                AddedBytes=compile(AddedFuntion, '<string>' ,'exec', 0,  False, 2)
                AddedBytesUsed=True
            except AttributeError:
                AddedFuntion=""
                AddedBytes=compile("", '<string>' ,'exec', 0,  True, 2)
                AddedBytesUsed=False
            
            del AddedFuntion

            collect()

            while True:
                start:int=log_time.time()

                if AddedBytesUsed:
                    exec(AddedBytes)

                wait(100 - (log_time.time() - start))
        
        def in_function_start(self) -> Thread:
            cla=Thread(self.in_function_auto_start)
            return cla

        def __enter__(self) -> Thread:
            self.Thread=self.in_function_start()
            return self.Thread

        def __exit__(self, ExcType, ExcVal, ExcTb) -> bool:
            if ExcType!=None:

                if ExcVal in self.OldExcVals:
                    return True
                
                self.OldExcVals.append(ExcVal)

                import uio
                if not _errorcreated:
                    brain.sdcard.savefile("Error.txt")
                exeption_string=uio.StringIO(500)
                sys.print_exception(ExcTb, exeption_string)  # type: ignore
                print(exeption_string.getvalue())
                brain.sdcard.appendfile("Error.txt", bytearray(b"<System ERROR> Python Runtime Error. \n %s "%(exeption_string.getvalue())))
            
                del exeption_string, uio
                return True
            else:
                self.Thread.stop()
                return False
            
    
    def Monitor(func):
        """
        Function decorator for using CLA in functions.
        """

        def Monitor_wrapper(*args, **kwargs):
            with IFCLA():
                func()
        
        return Monitor_wrapper
    
    def CLAStart() -> Thread:
        cla=Thread(log.auto_start)
        return cla

except Exception as e:
    import uio
    if not _errorcreated:
        brain.sdcard.savefile("Error.txt")
    exeption_string=uio.StringIO(500)
    sys.print_exception(e, exeption_string)  # type: ignore
    print(exeption_string.getvalue())
    brain.sdcard.appendfile("Error.txt", bytearray("<System ERROR> Python Runtime Error. \n %s "%(exeption_string.getvalue()), "utf-8"))
    
    del exeption_string, uio