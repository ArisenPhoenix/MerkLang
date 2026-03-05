class Types:
    def __init__(self):
        self.x = 1


class Square:
    def __init__(self, dim):
        self.list = [1]
        self.dict = {}
        self.types = Types()

    def set_types(self, y):
        self.types = y

    def print_stuff(self, x):
        if x != 0:
            self.print_stuff(x - 1)
        else:
            print(x)


square = Square(5)
square.list.append(50)
square.types.x = 20

z = square.types.x + square.list.pop(0)

types = Types()
types.x = 5
square.types = types
square.types.x += z

square.set_types(20)


def l():
    return 3


run = l
runni2ng = run()
