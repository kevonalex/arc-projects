# Generators
# Generators are used to 

x = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]

y = map(lambda i: i**2, x)

print(next(y))
print(next(y))
print(next(y))
print(next(y))
# or y.__next__()

print("For Loop Start")

for i in y:
    print(i)

while True:
    try:
        value = next(y)
    except StopIteration:
        print("Loop Completed.")
        break