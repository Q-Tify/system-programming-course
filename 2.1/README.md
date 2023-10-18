For my PC I changed this command in checker.py:
```
python test.py | exit 0
```
to
```
python3 test.py | exit 0
```

Useful commands:
```
gcc solution.c parser.h parser.c -Wextra -Werror -Wall -Wno-gnu-folding-constant
gcc solution.c parser.h parser.c ../utils/heap_help/heap_help.c -ldl -rdynamic 
python3 checker.py
```


Command for killing all hanged processes:
```
for pid in $(ps | awk 'NR > 1 {print $1}'); do kill "$pid"; done
```

Tests for 15 points:
```
echo "--------------------------------Section 1"
echo "$> Test 1"
mkdir testdir
echo "$> Test 2"
cd testdir
echo "$> Test 3"
pwd | tail -c 8
echo "$> Test 4"
   pwd | tail -c 8
echo "--------------------------------Section 2"
echo "$> Test 1"
touch "my file with whitespaces in name.txt"
echo "$> Test 2"
ls
echo "$> Test 3"
echo '123 456 " str "'
echo "$> Test 4"
echo '123 456 " str "' > "my file with whitespaces in name.txt"
echo "$> Test 5"
cat my\ file\ with\ whitespaces\ in\ name.txt
echo "$> Test 6"
echo "test" >> "my file with whitespaces in name.txt"
echo "$> Test 7"
cat "my file with whitespaces in name.txt"
echo "$> Test 8"
echo 'truncate' > "my file with whitespaces in name.txt"
echo "$> Test 9"
cat "my file with whitespaces in name.txt"
echo "$> Test 10"
echo "test 'test'' \\" >> "my file with whitespaces in name.txt"
echo "$> Test 11"
cat "my file with whitespaces in name.txt"
echo "$> Test 12"
echo "4">file
echo "$> Test 13"
cat file
echo "$> Test 14"
echo 100|grep 100
echo "--------------------------------Section 3"
echo "$> Test 1"
# Comment
echo "$> Test 2"
echo 123\
456
echo "--------------------------------Section 4"
echo "$> Test 1"
rm my\ file\ with\ whitespaces\ in\ name.txt
echo "$> Test 2"
echo 123 | grep 2
echo "$> Test 3"
echo 123\
456\
| grep 2
echo "$> Test 4"
echo "123
456
7
" | grep 4
echo "$> Test 5"
echo 'source string' | sed 's/source/destination/g'
echo "$> Test 6"
echo 'source string' | sed 's/source/destination/g' | sed 's/string/value/g'
echo "$> Test 7"
echo 'source string' |\
sed 's/source/destination/g'\
| sed 's/string/value/g'
echo "$> Test 8"
echo 'test' | exit 123 | grep 'test2'
echo "$> Test 9"
echo 'source string' | sed 's/source/destination/g' | sed 's/string/value/g' > result.txt
echo "$> Test 10"
cat result.txt
echo "$> Test 11"
yes bigdata | head -n 100000 | wc -l | tr -d [:blank:]
echo "$> Test 12"
exit 123 | echo 100
echo "$> Test 13"
echo 100 | exit 123
echo "$> Test 14"
printf "import time\n\
time.sleep(0.1)\n\
f = open('test.txt', 'w')\n\
f.write('Text\\\n')\n\
f.close()\n" > test.py
echo "$> Test 15"
python3 test.py | exit 0
echo "$> Test 16"
cat test.txt
```