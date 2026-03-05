i = 0
a = 1
b = 2
c = 0
n = 300000

while i < n:
    a = (a * 13 + 17) % 1000003
    b = (b + a + i) % 1000003

    if i % 7 == 0:
        c = c + (a % 97)
    else:
        c = c + (b % 89)

    i = i + 1

checksum = (a + b + c + i) % 1000003
print(checksum)
