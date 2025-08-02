# Decorators are used primarily to modify/extend the behaviour of functions and methods without changing their code.
# When using a Python decorator, you are wrapping the function with another function that takes the original function as an argument and returns its modified version.
# Decorators can be stacked due to their passing in of one function, and returning another function.
# The order in which decorators are applied affect their behaviour.

## SOURCE: https://realpython.com/primer-on-python-decorators/

# simple function example
def add_one(number: int):
    return number + 1

print(add_one(20))

# first-class objects
# functions can be passed into other functions as arguments

def say_hello(name):
    return f"Hello, {name}!"

def celebrate(name):
    return f"Hey {name}, we should celebrate!"

def greeter_function(greeting_func):
    return greeting_func("Bob")

output = greeter_function(say_hello)
output2 = greeter_function(celebrate)
print(output)
print(output2)


# functions as return values
## functions can be returned from other functions
def parent(num):
    def first_child():
        return "Hi, I'm Tobias!"
    
    def second_child():
        return "Hey, I'm Lukas."
    
    if num == 1:
        return first_child
    else:
        return second_child
    
first = parent(1) # returns a function
second = parent(2) # returns a function

print(first())
print(second())