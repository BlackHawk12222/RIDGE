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
import sys

try:
    from vex import *
    from gc import collect, mem_alloc# type: ignore 
    from ustruct import pack_into
    import uarray
    
    brain=Brain()
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
                    self.memory=0
                    self.currentMemory=0
                    self.memory_tolrance=int(str(settings.settings.get('memory_tolrance_KB ')))
                    self.aton=False
                    self.driver=False
                    self.comp_switch=False
                    self.field=False

                def memoryuse(self) -> None:
                    self.currentMemory=mem_alloc()/1000  # type: ignore
                    if not (self.memory >= self.currentMemory - self.memory_tolrance and self.memory <= self.currentMemory + self.memory_tolrance):
                        log.add("DSM0", str(self.currentMemory) + " KB")
                        self.memory=self.currentMemory
                
                def modules(self) -> None:
                    if self.modulelist != sys.modules:
                        filtered_list = [item for item in sys.modules if item not in self.modulelist]
                        log.add("DSP0", filtered_list)
                        self.modulelist= sys.modules.copy()

                def control(self, comp: Competition):
                    if comp.is_autonomous() and not self.aton:
                        log.add("DSC0", "")
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
                    self.setup={}
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

                def motor(self, motor: Motor|None=None) -> None:
                    """Capture for any general smart motor. Enter motor you wish to log as input. (Can take motor groups as well.)"""

                    if motor!=None and motor not in log.Motors:
                        log.Motors.append(motor)

                    for motor_ in log.Motors:
                        motor_id=str(motor_)
                        setup=self.setup
                        
                        # Setup id to sets if not there.
                        if motor_id not in setup:
                            self.motor_temp_monitoring[motor_id] = 0
                            self.motor_power_monitoring[motor_id] = 0
                            self.motor_current_monitoring[motor_id] = 0
                            self.motor_disconnected[motor_id] = False
                            self.setup[motor_id]=True

                        self.motor_temp:int=motor_.temperature(PERCENT)
                        self.current_motor_disconnected:int=self.motor_disconnected[motor_id]

                        if self.motor_temp==2:
                            if not self.current_motor_disconnected:
                                log.add("EM1", "%s"%(motor_))
                                self.motor_disconnected[motor_id]=True
                            else:
                                return
                        elif self.motor_temp!=2 and self.current_motor_disconnected:
                            self.motor_disconnected[motor_id]=False

                        self.current_motor_current_monitoring:int=self.motor_current_monitoring[motor_id]
                        self.current_motor_temp_monitoring:int=self.motor_temp_monitoring[motor_id]
                        self.current_motor_power_monitoring:int=self.motor_power_monitoring[motor_id]
                        self.current_motor_power:int=int(motor_.power(PowerUnits.WATT))
                        self.current_motor_current:int=int(motor_.current(CurrentUnits.AMP) * 10)
                        
                        # Cheaks for the temps,  power, and cheaks for conecttions of motors(s).
                        if self.motor_temp<=50: 
                            if self.current_motor_temp_monitoring>0:
                                log.add("DM0", "Motor %s, Temp %s"%(motor_, self.motor_temp))
                                self.motor_temp_monitoring[motor_id]=0
                        elif self.motor_temp>70: 
                            if (self.current_motor_temp_monitoring==0 or self.current_motor_temp_monitoring==2):
                                log.add("EM0", "Motor %s, Temp %s"%(motor_, self.motor_temp))
                                self.motor_temp_monitoring[motor_id]=1  
                        elif self.motor_temp>50: 
                            if self.current_motor_temp_monitoring==0:
                                log.add("WM0", "Motor %s, Temp %s"%(motor_, self.motor_temp))
                                self.motor_temp_monitoring[motor_id]=2
                        

                        if self.current_motor_power<=12: 
                            if self.current_motor_power_monitoring>0:
                                log.add("DM1", "Motor %s, Power %s"%(str(motor_), str(self.current_motor_power)))
                                self.motor_power_monitoring[motor_id]=0
                        elif self.current_motor_power>20: 
                            if (self.current_motor_power_monitoring==0 or self.current_motor_power_monitoring==2):
                                log.add("EM2", "Motor %s, Power %s"%(str(motor_), str(self.current_motor_power)))
                                self.motor_power_monitoring[motor_id]=1
                        elif self.current_motor_power>12: 
                            if self.current_motor_power_monitoring==0:
                                log.add("WM1", "Motor %s, Power %s"%(str(motor_), str(self.current_motor_power)))
                                self.motor_power_monitoring[motor_id]=2

                        if self.current_motor_current<=15: 
                            if self.current_motor_current_monitoring>0:
                                log.add("DM2", "Motor %s, Current %1.1f"%(str(motor_), float(self.current_motor_current)/10))
                                self.motor_current_monitoring[motor_id]=0
                        elif self.current_motor_current>20: 
                            if (self.current_motor_current_monitoring==0 or self.current_motor_current_monitoring==2):
                                log.add("EM3", "Motor %s, Current %1.1f"%(str(motor_), float(self.current_motor_current)/10))
                                self.motor_current_monitoring[motor_id]=1
                        elif self.current_motor_current>15:
                            if self.current_motor_current_monitoring==0:
                                log.add("WM2", "Motor %s, Current %1.1f"%(str(motor_), float(self.current_motor_current)/10))
                                self.motor_current_monitoring[motor_id]=2
                
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
                
                # Variables used to not have spam in log.  
                # self.battery_voltage_monitoring=0
                # self.battery_capacity_monitoring=0
                # self.battery_current_monitoring=0
                # self.battery_watt_monitoring=0
                # self.battery_temp_monitoring=0
                self.battery_values=uarray.array('H', [0, 0, 0, 0, 0])
                self.battery_monitoring=uarray.array('H', [0, 0, 0, 0, 0])
                # self.voltage:int=0
                # self.current:int=0
                # self.capacity:int=0
                self.watts:int=0
                self.axis={}
                self.button_objs=[]
                self.button_names = [
                    "A", "B", "X", "Y",
                    "UP", "DOWN", "LEFT", "RIGHT",
                    "L1", "L2", "R1", "R2",
                ]
                self.button_values = {}

            def battery(self) -> None:
                """
                Capture for the brains battery. 
                
                Args:
                None
                """
                

                self.battery_values[0]=int(brain.battery.voltage(VoltageUnits.VOLT))
                self.battery_values[1]=int(brain.battery.current(CurrentUnits.AMP) * 10)
                self.battery_values[2]=brain.battery.capacity()
                self.battery_values[3]=int(brain.battery.current(CurrentUnits.AMP) * int(brain.battery.voltage(VoltageUnits.VOLT)) / 10)
                self.battery_values[4]=int(brain.battery.temperature(PERCENT))

                # self.voltage:int=int(brain.battery.voltage(VoltageUnits.VOLT))
                # self.current:int=brain.battery.current(CurrentUnits.AMP)
                # self.capacity:int=brain.battery.capacity()
                # self.watts:int=int(brain.battery.current(CurrentUnits.AMP)) * int(brain.battery.voltage(VoltageUnits.VOLT))
                # self.temps:int=int(brain.battery.temperature(PERCENT))

                # Battery monitoring for voltage, capacity, and current.
                if self.battery_values[0]>=12:
                    if self.battery_monitoring[0]==1 or self.battery_monitoring[0]==2:
                        log.add("DB0", "%s"%(self.battery_values[0]))
                        self.battery_monitoring[0]=0
                elif self.battery_values[0]<12:
                    if self.battery_monitoring[0]==0 or self.battery_monitoring[0]==1:
                        log.add("WB0", "%s"%(self.battery_values[0]))
                        self.battery_monitoring[0]=2
                elif self.battery_values[0]<11:
                    if self.battery_monitoring[0]==0 or self.battery_monitoring[0]==2:
                        log.add("EB0", "%s"%(self.battery_values[0]))
                        self.battery_monitoring[0]=1

                if self.battery_values[2]>=50:
                    if self.battery_monitoring[2]!=self.battery_values[2]:
                        log.add("DB3", "%s"%(self.battery_values[2]))
                        self.battery_monitoring[2]=self.battery_values[2]
                elif self.battery_values[2]<50:
                    if self.battery_monitoring[2]!=self.battery_values[2]:
                        log.add("WB1", "%s"%(self.battery_values[2]))
                        self.battery_monitoring[2]=self.battery_values[2]
                elif self.battery_values[2]<25:
                    if self.battery_monitoring[2]!=self.battery_values[2]:
                        log.add("EB1", "%s"%(self.battery_values[2]))
                        self.battery_monitoring[2]=self.battery_values[2]

                if self.battery_values[1]<=10:
                    if self.battery_monitoring[1]==1 or self.battery_monitoring[1]==2:
                        log.add("DB1", "%s"%(self.battery_values[1]))
                        self.battery_monitoring[1]=0
                elif self.battery_values[1]>10:
                    if self.battery_monitoring[1]==0 or self.battery_monitoring[1]==1:
                        log.add("WB2", "%s"%(self.battery_values[1]))
                        self.battery_monitoring[1]=2
                elif self.battery_values[1]>15:
                    if self.battery_monitoring[1]==0 or self.battery_monitoring[1]==2:
                        log.add("EB2", "%s"%(self.battery_values[1]))
                        self.battery_monitoring[1]=1
                
                if self.battery_values[3]<=150:
                    if self.battery_monitoring[3]==1 or self.battery_monitoring[3]==2:
                        log.add("DB2", "%s"%(self.battery_values[3]))
                        self.battery_monitoring[3]=0
                elif self.battery_values[3]>150:
                    if self.battery_monitoring[3]==0 or self.battery_monitoring[3]==1:
                        log.add("WB3", "%s"%(self.battery_values[3]))
                        self.battery_monitoring[3]=2
                elif self.battery_values[3]>200:
                    if self.battery_monitoring[3]==0 or self.battery_monitoring[3]==3:
                        log.add("EB3", "%s"%(self.battery_values[3]))
                        self.battery_monitoring[3]=1

                if self.battery_values[4]<=30:
                    if self.battery_monitoring[4]==1 or self.battery_monitoring[4]==2:
                        log.add("DB4", "%s"%(self.battery_values[4]))
                        self.battery_monitoring[4]=0
                elif self.battery_values[4]>30:
                    if self.battery_monitoring[4]==0 or self.battery_monitoring[4]==1:
                        log.add("WB4", "%s"%(self.battery_values[4]))
                        self.battery_monitoring[4]=2
                elif self.battery_values[4]>50:
                    if self.battery_monitoring[4]==0 or self.battery_monitoring[4]==3:
                        log.add("EB4", "%s"%(self.battery_values[4]))
                        self.battery_monitoring[4]=1  

            def controller(self, controller: Controller) -> None:
                """
                Capture for the controllers. 
                Enter controller you wish to log. 
                
                Args:
                controller= Controller()
                """
                controllerid=str(controller)

                if controllerid not in self.button_values:
                    self.button_values[controllerid]=[True,True,True,True,True,True,True,True,True,True,True,True]

                
                if controllerid not in self.axis:
                    self.axis[controllerid]=[0, 0, 0, 0]

                self.ctrl_name = str(controller)
                self.c_axis1 = controller.axis1.position()
                self.c_axis2 = controller.axis2.position()
                self.c_axis3 = controller.axis3.position()
                self.c_axis4 = controller.axis4.position()

                if self.c_axis1 != 0 and not (self.axis[controllerid][0] >= self.c_axis1 - log.tolrance and self.axis[controllerid][0] <= self.c_axis1 + log.tolrance):
                    log.add("DC1", "%s, Axis1, %d"%(self.ctrl_name, self.c_axis1))
                    self.axis[controllerid][0] = self.c_axis1
                elif self.c_axis1 == 0 and self.axis[controllerid][0] != 0:
                    log.add("DC1", "%s, Axis1, %d"%(self.ctrl_name, 0))
                    self.axis[controllerid][0] = 0

                if self.c_axis2 != 0 and not (self.axis[controllerid][1] >= self.c_axis2 - log.tolrance and self.axis[controllerid][1] <= self.c_axis2 + log.tolrance):
                    log.add("DC1", "%s, Axis2, %d"%(self.ctrl_name, self.c_axis2))
                    self.axis[controllerid][1] = self.c_axis2
                elif self.c_axis2 == 0 and self.axis[controllerid][1] != 0:
                    log.add("DC1", "%s, Axis2, %d"%(self.ctrl_name, 0))
                    self.axis[controllerid][1] = 0

                if self.c_axis3 != 0 and not (self.axis[controllerid][2] >= self.c_axis3 - log.tolrance and self.axis[controllerid][2] <= self.c_axis3 + log.tolrance):
                    log.add("DC1", "%s, Axis3, %d"%(self.ctrl_name, self.c_axis3))
                    self.axis[controllerid][2] = self.c_axis3
                elif self.c_axis3 == 0 and self.axis[controllerid][2] != 0:
                    log.add("DC1", "%s, Axis3, %d"%(self.ctrl_name, 0))
                    self.axis[controllerid][2] = 0

                if self.c_axis4 != 0 and not (self.axis[controllerid][3] >= self.c_axis4 - log.tolrance and self.axis[controllerid][3] <= self.c_axis4 + log.tolrance):
                    log.add("DC1", "%s, Axis4, %d"%(self.ctrl_name, self.c_axis4))
                    self.axis[controllerid][3] = self.c_axis4
                elif self.c_axis4 == 0 and self.axis[controllerid][3] != 0:
                    log.add("DC1", "%s, Axis4, %d"%(self.ctrl_name, 0))
                    self.axis[controllerid][3] = 0

                # Button logging for controller.

                
                self.button_objs = [
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
                

                for i in range(12):
                    if self.button_objs[i].pressing():
                        if self.button_values[controllerid][i]:
                            log.add("DC0", "%s, %s, Pressed"%(self.ctrl_name, self.button_names[i]))
                            self.button_values[controllerid][i] = False
                    else:
                        if not self.button_values[controllerid][i]:
                            log.add("DC0", "%s, %s, Released"%(self.ctrl_name, self.button_names[i]))
                            self.button_values[controllerid][i] = True

            def variable(self, name: str, value: Any) -> None:
                """
                Capture for int, float, and bool variables. 
                Enter name of variable in a string and then the variable you wish to log.
                
                Args:
                name= String
                value= Int, Boolean, Float
                """

                self.valueid=str(name)

                # Adds id if not in set.
                if self.valueid not in self.variables:

                    if type(value)==bool:
                        self.variables[self.valueid]=False
                    else:
                        self.variables[self.valueid]=0
                
                if value != self.variables[self.valueid]:
                    log.add("DV0", "%s, Val %s"%(name, value))
                    self.variables[self.valueid] = value

    class Log:
        """Main object for the CLEAR import. \n To start logging use the "logstart()" function in this object to do the main logging if you need help with its inputs use help() over the "logstart()" function."""
                    
        def __init__(self):
            if not brain.sdcard.exists("loghistory.txt"):
                brain.sdcard.savefile("loghistory.txt")
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
            self.Motors: list[Motor]=[]
            self.robot_active=False

            brain.sdcard.savefile("Logstart.txt")  # Clears Logstart file for refresh of instructions in it.

            # Predefined Log Codes dictionary
            self.codes:dict={
            """Main dictionary for CLEAR"""
                    "EB0": "<Batt ERR: Very Low Volt>",
                    "EB1": "<Batt ERR: Very Low Batt>",
                    "EB2": "<Batt ERR: Very High Curr>",
                    "EB3": "<Batt ERR: Very High Watt>",
                    "EB4": "<Batt ERR: Very High Temp>",
                    "WB0": "<Batt WARN: Low Volt>",
                    "WB1": "<Batt WARN: Low Batt>",
                    "WB2": "<Batt WARN: High Curr>",
                    "WB3": "<Batt WARN: High Watt>",
                    "WB4": "<Batt WARN: High Temp>",
                    "DB0": "<Batt DATA: Volt Normal>",
                    "DB1": "<Batt DATA: Curr Normal>",
                    "DB2": "<Batt DATA: Watt Normal>",
                    "DB3": "<Batt DATA: Capacity Changed>",
                    "DB4": "<Batt DATA: Temp Normal>",
                    "DA1": "<Aton DATA: Recording Stopped>",
                    "DA2": "<Aton DATA: Recording Saved>",
                    "DA3": "<Aton DATA: Recording Loaded>",
                    "DS0": "<System DATA: Init setup Done>",
                    "DS1": "<System DATA: Archived Log>",
                    "DS2": "<System DATA: Index Log History Done>",
                    "DS3": "<System DATA: Archived Recording>",
                    "DS5": "<System DATA: Recalled Recording>",
                    "EM0": "<Motor ERR: Very Hot>",
                    "EM1": "<Motor ERR: Disconnected>",
                    "EM2": "<Motor ERR: Very High Power>",
                    "EM3": "<Motor ERR: Very High Current>",
                    "WM0": "<Motor WARN: Hot>",
                    "WM1": "<Motor WARN: High Power>",
                    "WM2": "<Motor WARN: High Current>",
                    "DM0": "<Motor DATA: Temp Normal>",
                    "DM1": "<Motor DATA: Power Normal>",
                    "DM2": "<Motor DATA: Current Back To Normal>",
                    "DV0": "<Variable DATA: Changed>",
                    "DC0": "<Cont DATA: Button Changed>",
                    "DC1": "<Cont DATA: Axis Changed>",
                    "DPW0": "<Pwm DATA: Value Changed>",
                    "DP0": "<Pot DATA: Value Changed>",
                    "DLS1": "<Limit DATA: Released>",
                    "DLS0": "<Limit DATA: Pressed>",
                    "DO3": "<Optical DATA: Installed>",
                    "EO0": "<Optical ERROR: Disconnected>",
                    "DO1": "<Optical DATA: Detected Object>",
                    "DO0": "<Optical DATA: Color Changed. Color:",
                    "DO2": "<Optical DATA: Lost Object>",
                    "DI7": "<Inertial DATA: Installed>",
                    "DI2": "<Inertial DATA: Calibrating>",
                    "DI3": "<Inertial DATA: Calibration Complete>",
                    "DI0": "<Inertial DATA: Rotation Changed>",
                    "DI9": "<Inertial DATA: Roll Changed>",
                    "DI8": "<Inertial DATA: Pitch Changed>",
                    "DI1": "<Inertial DATA: Heading Changed>",
                    "DI4": "<Inertial DATA: X Axis Changed>",
                    "DI5": "<Inertial DATA: Y Axis Changed>",
                    "DI6": "<Inertial DATA: Z Axis Changed>",
                    "EI0": "<Inertial ERROR: Disconnected>",
                    "DDS3": "<Distance DATA: Installed>",
                    "EDS0": "<Distance ERROR: Disconnected>",
                    "DDS0": "<Distance DATA: Detected Object>",
                    "DDS1": "<Distance DATA: Distance Changed>",
                    "DDS4": "<Distance DATA: Lost Object>",
                    "DR0": "<Rotation DATA: Installed>",
                    "ER0": "<Rotation ERROR: Disconnected>",
                    "DR1": "<Rotation DATA: Angle Changed>",
                    "DR2": "<Rotation DATA: Position Changed>",
                    "DDI0": "<Digital DATA: High>",
                    "DDI1": "<Digital DATA: Low>",
                    "DAI0": "<Analog DATA: Changed Value>",
                    "DBS0": "<Bumper DATA: Pressed>",
                    "DBS1": "<Bumper DATA: Released>",
                    "DSM0": "<Mem DATA: Memory Useage Changed> ",
                    "DSP0": "<Sys DATA: New module(s) added>",
                    "DSC0": "<Comp DATA: Aton Started>",
                    "DSC1": "<Comp DATA: Driver Started>",
                    "DSC2": "<Comp DATA: Connected>",
                    "DSC3": "<Comp DATA: Field Connected>",
                    "DSC4": "<Comp DATA: Field Disconnected>",
                    "DSC5": "<Comp DATA: Disconnected>"
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
                self._cache.extend(b"%d [%d], %s, %s\n" % (self._index, log_time.time(), self.codes.get(add_code), add_details))
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
                
            self.entry=b"%d [%d ms], %s, %s\n" %(self._index, log_time.time(), self.codes.get(add_code), add_details)
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
            
            self.format:str=str(settings.settings.get('format_used '))
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

            if self.brainscreen:
                brain.screen.set_font(FontType.MONO12)

            # Logs system start.
            self.add("DS0", "")
            
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

            controllers: List[Controller]=[]

            globallogging=dir()

            #print(globallogging)

            for item in globallogging:
                
                try:
                    item_type=str(type(eval(item)))
                except NameError:
                    continue

                if  (item_type == "<class 'int'>" or item_type == "<class 'bool'>" or item_type == "<class 'float'>") and auto_do_variables:
                    log.add_logstart("log.capture.variable('%s', %s)"%(item, item.replace("'", "")))
                elif item_type == "<class 'motor'>" and auto_do_motors:
                    self.Motors+=[eval(item)]
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

            print("Logstart: ")
            print(brain.sdcard.loadfile("Logstart.txt").decode(self.format))

            _exec=exec
            lwait=wait
            capture_memory=self.capture.system.memoryuse
            capture_modules=self.capture.system.modules
            capture_battery=log.capture.battery
            timer=log_time.time
            gc_collect=collect
            motorcapture=log.capture.smartport.motor
            controllercapture=log.capture.controller
            log_check=log.append_log

            # Loads extra funtions from file.
            try:
                addedfuntion=brain.sdcard.loadfile("Logstart.txt").decode(self.format)
                added_bytes=compile(addedfuntion, '<string>' ,'exec', 0,  False, 2)
                added_bytes_used=True
            except AttributeError:
                addedfuntion=""
                added_bytes=compile("", '<string>' ,'exec', 0,  True, 2)
                added_bytes_used=False
            
            del addedfuntion
            
            gc_collect()

            while True:
                for _ in range(20):
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
                        _exec(added_bytes)
                    
                    if self.Motors:
                        motorcapture()

                    if log_memory:
                        capture_memory()

                    if log_modules:
                        capture_modules()
                    
                    if log_battery:
                        capture_battery()
                    
                    print(timer()-start)

                    lwait(wait_time_logging - (timer() - start))

                if gc_use:
                    gc_collect()

        def __enter__(self) -> Thread:
            self.Thread=Thread(log.auto_start)
            return self.Thread

        def __exit__(self) -> None:
            self.Thread.stop()

    class Encode:
        def __init__(self):
            self.axis2History=0
            self.axis3History=0
            self.degreesoffsetright=0
            self.degreesoffsetleft=0
            self.codeobjectoffset=0

        def rightmoveCLEAR(self, Motors: List[Motor], Velocity: int):
            for motor in Motors:
                motor.set_velocity(Velocity, PERCENT)
            
        def leftmoveCLEAR(self, Motors: List[Motor], Velocity: int):
            for motor in Motors:
                motor.set_velocity(Velocity, PERCENT)
        
        def rightDegreesCLEAR(self, Motors: List[Motor], degrees):
            for motor in Motors:
                motor.spin_for(FORWARD, degrees, DEGREES)

        def leftDegreesCELAR(self, Motors: List[Motor], degrees):
            for motor in Motors:
                motor.spin_for(FORWARD, degrees, DEGREES)

        def run(self):
            pass

        def encode(self, Name: str, RightMotors: List[Motor], LeftMotors: List[Motor], dotank=True):

            prelist:List[List[str]]=[]
            file=brain.sdcard.loadfile("RecordingData.csv").decode(log.format).split("\n")
            for line in file:
                if line:
                    prelist.append(line.split(','))

            codeobject=bytearray(50000)
            
            for i in range(len(prelist)):
                axis1=int(prelist[i][0].replace("A1", ""))
                axis2=int(prelist[i][1].replace("A2", ""))
                axis3=int(prelist[i][2].replace("A3", ""))
                axis4=int(prelist[i][3].replace("A4", ""))
                timestamp=int(prelist[i][4].replace("T", ""))
                rightposition=int(prelist[i][6].replace("RP", ""))
                leftposition=int(prelist[i][7].replace("LP", ""))
                pressedbuttons: List[str] = prelist[i][5].replace("BP", "").split(":")
                print(axis1, axis2, axis3, axis4, timestamp, pressedbuttons, rightposition, leftposition)

                if dotank:
                    codestring=b", CLEAR.encode.rightmoveCLEAR(%s, %s), CLEAR.encode.leftmoveCLEAR(%s, %s)"%(RightMotors, axis2, LeftMotors, axis3)
                else:
                    pass

                if axis2 > 0:
                    axis2sign=True
                else:
                    axis2sign=False
                
                if axis3 > 0:
                    axis3sign=True
                else:
                    axis3sign=False

                codestringdegreeright=b""
                codestringdegreeleft=b""
                
                if axis2sign != self.axis2History:
                    self.axis2History=axis2sign
                    degrees=rightposition-self.degreesoffsetright
                    codestringdegreeright=b", rightDegreesCLEAR(%s, %s)"%(RightMotors, degrees)
                    self.degreesoffsetright=rightposition

                if axis3sign != self.axis3History:
                    self.axis3History=axis3sign
                    degrees=leftposition-self.degreesoffsetleft
                    codestringdegreeleft=b", leftDegreesCLEAR(%s, %s)"%(LeftMotors, degrees)
                    self.degreesoffsetright=rightposition
                
                fullstring="%s%s%s"%(codestring, codestringdegreeright, codestringdegreeleft)
                
                stringsize=len(fullstring)
                pack_into("=%ds"%(stringsize), codeobject, self.codeobjectoffset, fullstring)

                self.codeobjectoffset+=stringsize 

            brain.sdcard.savefile(Name, codeobject[0:self.codeobjectoffset])     
    
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
    class Recording:
        """
        Main class for recording.
        """

        def __init__(self):
            self.record=False

        def start(self, controller:Controller, Right: Motor, Left: Motor):
            brain.sdcard.savefile("RecordingData.csv")
            self.record=True
            self._record_loop(controller, Right, Left)

        def stop(self, Name, Right, Left):
            self.record=False
            encode.encode(Name, Right, Left)

        def _record_loop(self, controller: Controller, Right: Motor, Left: Motor):
            while True:
                
                if not self.record:
                    break
                
                self.axis1=controller.axis1.position()
                self.axis2=controller.axis2.position()
                self.axis3=controller.axis3.position()
                self.axis4=controller.axis4.position()
                self.time_stamp=log_time.time()
                self.buttonspressed=bytearray()

                if controller.buttonA.pressing():
                    self.buttonspressed.extend(b"A: ")

                if controller.buttonB.pressing():
                    self.buttonspressed.extend(b"B: ")

                if controller.buttonX.pressing():
                    self.buttonspressed.extend(b"X: ")

                if controller.buttonY.pressing():
                    self.buttonspressed.extend(b"Y: ")

                if controller.buttonUp.pressing():
                    self.buttonspressed.extend(b"Up: ")

                if controller.buttonDown.pressing():
                    self.buttonspressed.extend(b"Down: ")

                if controller.buttonLeft.pressing():
                    self.buttonspressed.extend(b"Left: ")

                if controller.buttonRight.pressing():
                    self.buttonspressed.extend(b"Right: ")

                if controller.buttonR1.pressing():
                    self.buttonspressed.extend(b"R1: ")

                if controller.buttonR2.pressing():
                    self.buttonspressed.extend(b"R2: ")

                if controller.buttonL1.pressing():
                    self.buttonspressed.extend(b"L1: ")

                if controller.buttonL2.pressing():
                    self.buttonspressed.extend(b"L2: ")

                brain.sdcard.appendfile("RecordingData.csv", bytearray(b"%d A1, %d A2, %d A3, %d A4, %d T, %s BP, %d RP, %d LP \n"%(self.axis1, self.axis2 , self.axis3, self.axis4, self.time_stamp , self.buttonspressed.decode("utf-8"), Right.position(DEGREES), Left.position(DEGREES))))

                wait(20, MSEC)
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
    encode=Encode()
    recording=Recording()

except Exception as e:
    import uio
    if not _errorcreated:
        brain.sdcard.savefile("Error.txt")
    exeption_string=uio.StringIO(500)
    sys.print_exception(e, exeption_string)  # type: ignore
    print(exeption_string.getvalue())
    brain.sdcard.appendfile("Error.txt", bytearray(b":System ERROR: Runtime Error.: \n %s "%(exeption_string.getvalue()))) 
    
    del exeption_string, uio