# Modified producer-consumer problem

One producer, three different consumers, one limited buffer.

There are three consumers (1, 2, 3). Element is deleted from the buffer after read by consumer 1 and 2, or 2 and 3 (2 and 1, 3 and 2 also). Every consumer can read each element only once.

## Running

```
$ make
$ ./project
```

You can also enter program's duration. Just type in a number instead of "x".

```
$ ./project x
```