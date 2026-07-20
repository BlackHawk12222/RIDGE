from vex import *

import CLA, Interface, RE, OD

class V5TK:
    @staticmethod
    def RunAll() -> Thread:
        V5TKThread=CLA.log.auto_start()
        return V5TKThread