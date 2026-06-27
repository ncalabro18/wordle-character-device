
__The wait is over__: Wordle, as a Linux character device.

### Quick Start Guide:

Step 1: 

&nbsp;&nbsp;Write your own shell 🐚. (busybox exists, I'm not accepting any more customer complaints, fish are supposed to swim)

Step 2 (optional):

&nbsp;&nbsp;Spin up a RISC-V RHEL Podman container (subscription not included). 

Step 3:

&nbsp;&nbsp;Mount 🐎 the device.

Step 4:

&nbsp;&nbsp;Play via cat and printf against the mounted device file, like a NORMAL person. (cat and printf not included, see step 1)

Step 5:

&nbsp;&nbsp;I am not responsible for any damaged hardware. Those who skip step 2 have my full moral support and absolutely nothing else.

Step 6:

&nbsp;&nbsp;Don’t panic – wordle will handle that 👍 

### Wordle Feedback Encoding Table

Feedback is returned in a highly intuitive format that will feel immediately familiar:

| Value | Meaning |
|-------|---------|
| `0` | Letter not in word |
| `1` | Letter in word, wrong position |
| `2` | Letter in correct position |

