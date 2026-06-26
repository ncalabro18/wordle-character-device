
### Instructions

 - Create a Podman container and clone the repository inside.

 - Set your favorite shell as the init process.

 - Run ```make run```

 - Add the module to the Kernel runtime: ```insmod driver.ko```

 - ```cd /dev``` - device file is located at /dev/wordle

 - To enter a word, use ``printf 5_letter_word > wordle```

 - Show results via ```cat wordle```


 ### ASCII Legend

| Value | Meaning |
|-------|---------|
| `0` | Letter not in word |
| `1` | Letter in word, wrong position |
| `2` | Letter in correct position |

A result of `22222` means you won. `33333` means you lost.

You have **5 guesses**. Only uppercase letters are accepted.


## Controls

| Operation | Effect |
|-----------|--------|
| `lseek(SEEK_SET)` | Reset the game with a new secret word |
| `lseek(SEEK_CUR)` | Returns the number of guesses remaining |

