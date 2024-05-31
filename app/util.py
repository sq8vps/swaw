# Convert boolean to int: true to 1, false to 0
def btoi(b: bool) -> int:
    return 1 if b == True else 0

def itob(i: int) -> bool:
    return True if i > 0 else False