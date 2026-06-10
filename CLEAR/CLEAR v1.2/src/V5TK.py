from vex import *

import CLA, Interface, RE, OD

class V5TK:
    @staticmethod
    def RunAll() -> Thread:
        V5TKThread=CLA.log.start()
        return V5TKThread