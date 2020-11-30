import math
import hashlib
import re

#/*------------------------------------------------------------------------------------------------*/
# Last digits
# sum = 0
# for i in range(1, 1001):
#     sum += i**i
#     print(sum)
#     print('\n')

# print(sum)

#/*------------------------------------------------------------------------------------------------*/
# Palindrome sums
# def is_palindrome(inp):
#     string = str(inp)
#     return string == string[::-1]

# total = 0
# for i in range(0, 100000):
#     b1 = str(bin(i))[2:]
#     b2 = str(oct(i))[2:]
#     b3 = str(hex(i))[2:]

#     if is_palindrome(b1) and is_palindrome(b2) and is_palindrome(b3):
#         print(b1)
#         print(b2)
#         print(b3)
#         print(i)
#         total += i
#         print('\n')

# print(total)

#/*------------------------------------------------------------------------------------------------*/
# COINS
# ways = 0
# coins = [1, 2, 5, 10, 20, 50, 100, 200]

# sum = 200

# def fill(current, highest_available_i):
#     global ways
#     global coins
#     global sum

#     if highest_available_i == -1:
#         return

#     fill(current, highest_available_i - 1)
    
#     while current + coins[highest_available_i] <= sum:
#         current += coins[highest_available_i]

#         if current == sum:
#             ways = ways + 1
#         else:
#             fill(current, highest_available_i - 1)

# fill(0, len(coins) - 1)
# print(ways)

#/*------------------------------------------------------------------------------------------------*/
# Rabbits
# labbits = []
# for m in range(1, 3*12 + 1):
#     if m == 1 or m == 2:
#         labbits.append(1)
#     else:
#         labbits.append(labbits[m - 3] + labbits[m - 2])

#     if m > 12:
#         labbits[-1] = labbits[-1] - labbits[-13]
        
#     print(m)
#     print(labbits[-1])
#     print('\n')