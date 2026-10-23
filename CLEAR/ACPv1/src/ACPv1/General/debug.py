from vex import *
import sys

utime = Timer()

"""Used for debugging functions in code."""

def debug_start(func):
    """Does error handling, prints all variables, and gives time fof funtion."""

    def wrapper(*args, **kwargs):
        try:
            speed:float=utime.time()
            print("Startingdebug for %s"%(func))
            func(*args, **kwargs)
            print(func)
            print("Time of function: %f Msec"%(utime.time() -speed))
            print(locals())
        except Exception as e:
            sys.print_exception(e) # type: ignore
    return wrapper

def debug_handling(func):
    """Puts in error handling."""

    def wrapper(*args, **kwargs):
        try:
            func(*args, **kwargs)
        except Exception as e:
            sys.print_exception(e) # type: ignore
    return wrapper

def debug_print(func):
    """Prints variables."""

    def wrapper(*args, **kwargs):
        print("Startingdebug for %s"%(func))
        func(*args, **kwargs)
        print(func)
        print(locals())
    return wrapper

def debug_time(func):
    """Prints time to exec the function."""

    def wrapper(*args, **kwargs):
        speed:float=utime.time()
        func(*args, **kwargs)
        print("Time of function: %f Msec"%(utime.time() - speed))
    return wrapper