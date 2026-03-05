x = 0
y = 3
z = x + y

if z == 13:
    z = 10
else:
    z = 4

count = 0

hello = "hello"
world = "world"
hello_world = hello + " " + world

while x < 1:
    x = x + 1
    if x == 1:
        if count == 0:
            count = count + 1
            x = 99
    if x == 99:
        if count == 1:
            x = 101


def run():
    x = 2
    while x > 1:
        x = x - 1
    return x


final_x = run()


def go():
    x = 20
    while x > 10:
        x = x - 1
        if x < 15:
            break
    return x


final_x_2 = go()

while final_x_2 < 20:
    final_x_2 += 3
    if final_x_2 == 20:
        final_x_2 = 21
        continue


def no_return():
    x = 20
    return x


hello = "Hello World"
not_has_hellow = "Hellow" in hello
has_hello = "Hello" in hello


class Run:
    def __init__(self):
        x = 20

    def runs(self, x):
        return 3 + x


runner = Run()
runner.runs(20)

two = 2.0
if two == 2:
    print("hi")
if two == 2:
    print("hi")
