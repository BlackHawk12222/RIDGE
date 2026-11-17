class CompFilter():
    def __init__(self, Alpha: float = 0.98, dt_SECONDS: float = 0.01) -> None:
        self.alpha = Alpha
        self.DT_SECONDS = dt_SECONDS
        self.Output = 0.0

    def __call__(self) -> float:
        return self.Output
    
    # --- Allow float(kf), print(kf), and f-strings ---
    def __float__(self) -> float:
        return float(self.Output)

    def __repr__(self) -> str:
        return repr(self.Output)

    # --- Comparisons (<, <=, ==, !=, >, >=) ---
    def __lt__(self, other): return self.Output < float(other)
    def __le__(self, other): return self.Output <= float(other)
    def __eq__(self, other): return self.Output == float(other)
    def __ne__(self, other): return self.Output != float(other)
    def __gt__(self, other): return self.Output > float(other)
    def __ge__(self, other): return self.Output >= float(other)

    # --- Math operations (+, -, *, /, etc.) ---
    def __add__(self, other): return self.Output + float(other)
    def __radd__(self, other): return float(other) + self.Output
    def __sub__(self, other): return self.Output - float(other)
    def __rsub__(self, other): return float(other) - self.Output
    def __mul__(self, other): return self.Output * float(other)
    def __rmul__(self, other): return float(other) * self.Output
    def __truediv__(self, other): return self.Output / float(other)
    def __rtruediv__(self, other): return float(other) / self.Output
    def __neg__(self): return -self.Output
    def __abs__(self): return abs(self.Output)

    def update(self, FastData: float, SlowData: float) -> None:
        """Updates Output. First input must be change, Second must be absolute."""

        self.Output = (self.alpha * (self.Output + FastData * self.DT_SECONDS)) + ((1.0 - self.alpha) * SlowData)