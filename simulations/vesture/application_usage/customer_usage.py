# Script for simulating customer usage data for training the ML recommendation model.
import pandas as pd
import random
import logging

# SET LOGGING CONFIG
# logging.basicConfig(level=logging.INFO)
logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger(__name__)

#  BEGIN CODE
# generate explicit training data
# generate customers csv file
# generate random number of customers between 100 - 200
customer_count = random.randint(100, 200)
logger.info(f"Customer count: {customer_count}")

# generate products csv file

# generate customer and product interactions csv file (implicit data for training use)