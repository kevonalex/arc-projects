## Code demonstration of how to use 'try-except' blocks to handle exceptions in the code. 

import os

script_directory = os.path.dirname(os.path.abspath(__file__))
file_path = os.path.join(script_directory,'resources','data','exceptions_demo_text.txt')
print(file_path)
# begin using code that you want to handle errors for
try:
    # f = open('tutorials/code_quality/resources/data/exceptions_demo_text.txt') # incorrect
    f = open(file_path) # correct
    name = "test_file_1"
    print(f"File name: {f.name}")
    if f.name == r"c:\dev\Projects\Arc\arc-projects\tutorials\code_quality\resources\data\exceptions_demo_text.txt":
        raise Exception
# capture specific errors that are likely to appear
except FileNotFoundError: # start with more specific exceptions first, then move to more general exceptions.
    print("File does not exist.")
except NameError:
    print("Naming error within code block.")
# capture generic exceptions here
except Exception as e:
    print("Something went wrong.")
# else only runs if no exceptions are invoked by the code
else:
    print("No errors loading file. Extracting data.")
    print(f"File contents: \n\n {f.read()}\n")
    f.close()
# finally blocks will run regardless of whether an exception has been generated or not. This would be used to close database connections, clear caches, etc.
finally:
    print("All work requiring file has been completed.")