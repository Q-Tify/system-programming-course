## Solution with 2 bonus tasks
To run the code and check for leaks:
```
gcc solution.c libcoro.c ../utils/heap_help/heap_help.c -ldl -rdynamic

./a.out 1000 2 test1.txt test2.txt test3.txt test4.txt test5.txt test6.txt
```
To check the output file:
```
python3 checker.py -f merged_array.txt
```