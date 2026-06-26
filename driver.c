#include <linux/fs.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/major.h>
#include <linux/blkdev.h>
#include <linux/cdev.h>
#include <linux/err.h>
#include <linux/miscdevice.h>



#define WORDLE_WIN_STRING  "22222"
#define WORDLE_LOSE_STRING "33333"


/*** wordle helper declarations ***/

static void wordle_state_reset(void);
static char* wordle_secret_word(void);
static int wordle_contains(char c);

// no libc:
static int wordle_strlen(const char *);
static int wordle_isupper(char c);
static void wordle_strncpy(char *dest, char *src, int n);
static int wordle_strncmp(char *str1, char *str2, int n);




/* file_operation callbacks declarations */
static loff_t  wordle_llseek (struct file *, loff_t, int);
static ssize_t wordle_read   (struct file *, char __user *, size_t, loff_t *);
static ssize_t wordle_write  (struct file *, const char __user *, size_t, loff_t *);
static int     wordle_open   (struct inode *, struct file *);

enum GameStatus {
	GS_WIN,
	GS_LOSE,
	GS_IN_PROGRESS
};


struct WordleState {
	char      *secret_word;
	enum GameStatus game_status;
	int        guess_count;
	char      feed_back[5];
};

static struct WordleState *wordle_state = NULL;


/* 100 words for solution pool */
static char *wordle_words[] = {
	"DREAM", "COUNT", "RIVAL", "CLEAR", "MOUNT", "DRAWN", "BADLY", "ORDER",
	"RURAL", "ENJOY", "SHIFT", "SPEAK", "STOOD", "VOICE", "LARGE", "LOCAL",
	"SUPER", "FIRST", "WRONG", "SOUND", "WORST", "GIANT", "HORSE", "STAGE",
	"HUMAN", "UNITY", "EMPTY", "HEAVY", "LAYER", "GREAT", "ALARM", "MAYOR",
	"STAND", "THING", "PETER", "FALSE", "OCCUR", "ALONG", "BASIC", "ALLOW",
	"JAPAN", "GLOBE", "TRIES", "LEAVE", "BROKE", "ALIKE", "CYCLE", "LIVES",
	"BRAIN", "SHARP", "NEEDS", "STILL", "ADULT", "TRUTH", "HOUSE", "BEACH",
	"GROWN", "SENSE", "EQUAL", "METAL", "CRASH", "CHILD", "TOPIC", "MUSIC",
	"GRAND", "HAPPY", "RAISE", "JONES", "NOVEL", "DAILY", "SHELL", "TOUCH",
	"USUAL", "FIGHT", "ASIDE", "ANGLE", "DRILL", "MEDIA", "FLOOR", "TITLE",
	"THICK", "SIXTY", "PARTY", "EXIST", "THANK", "ENTER", "RADIO", "ALONE",
	"WORLD", "FOUND", "BIRTH", "SIXTH", "GRACE", "SHIRT", "SMALL", "GLASS",
	"UNDUE", "CLEAN", "HARRY", "APPLE"
};

static struct file_operations wordle_file_operations = {
	.owner  =   THIS_MODULE,
	.read   =   wordle_read,
	.write  =  wordle_write,
	.open   =   wordle_open,
	.llseek = wordle_llseek
};

static struct miscdevice wordle_miscdevice = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name           = "wordle",
	.fops		= &wordle_file_operations,
};


/*** file_operation callbacks definitions ***/

static loff_t  wordle_llseek (struct file *f, loff_t offset, int num) {


	if (num == SEEK_CUR) {
		f->f_pos++;
		return wordle_state->guess_count;
	} else if (num == SEEK_SET) {
		wordle_state_reset();
	}
	return offset;
}

static ssize_t wordle_read   (struct file *f, char __user *buf, size_t size, loff_t *offset) {
	int  win_str_len = wordle_strlen (WORDLE_WIN_STRING);
	int lose_str_len = wordle_strlen(WORDLE_LOSE_STRING);

	int err = 0;
	if (*offset >= 5)
		return 0;

	*offset += 5;

	switch (wordle_state->game_status) {

		case GS_WIN:
			err = copy_to_user(buf, WORDLE_WIN_STRING, win_str_len);

			if (err != 0) {
				printk ("wordle_read: copy_to_user error\n");
				return -EINVAL;
			}
			return 5;

		case GS_LOSE:
			err = copy_to_user(buf, WORDLE_LOSE_STRING, lose_str_len);

			if (err != 0) {
				printk ("wordle_read: copy_to_user error\n");
				return -EINVAL;
			}
			return 5;

		case GS_IN_PROGRESS:
			err = copy_to_user(buf, wordle_state->feed_back, 5);

			if (err != 0) {
				printk ("wordle_read: copy_to_user error\n");
				return -EINVAL;
			}

			return 5;
	}


	// switch must always return, consider this a default section
	printk ("wordle_read: invalid state\n");
	return -EINVAL;
}

static ssize_t wordle_write  (struct file *f, const char __user *buf, size_t len, loff_t *offset) {
	char local_buf[6] = "\0\0\0\0\0\0";

	if (wordle_state->game_status == GS_WIN || wordle_state->game_status == GS_LOSE)
		return -EIO;


	if (len != 5) {
		printk("wordle_write: len !=5; %lu\n", len);
		return -EINVAL;
	}

	int ret = copy_from_user(local_buf, buf, 5);

	if (ret < 0) {
		printk("wordle_write: copy_from_user error %d\n", ret);
		return -EFAULT;
	}

	// check characters are valid
	for (int i = 0; i < 5; i++)
		if ( !wordle_isupper( local_buf[i])) {
			printk("wordle_write: invalid character written to wordle\n");
			return -EINVAL;
		}

	wordle_state->guess_count--;

	if (wordle_strncmp(wordle_state->secret_word, local_buf, 5) == 0) {
	    wordle_state->game_status = GS_WIN;
	    wordle_strncpy(wordle_state->feed_back, WORDLE_WIN_STRING, 5);
	} else if (wordle_state->guess_count <= 0) {
	    wordle_state->game_status = GS_LOSE;
	    wordle_strncpy(wordle_state->feed_back, WORDLE_LOSE_STRING, 5);
	} else {
	    // Only score per-character when the game is still in progress
	    for (int i = 0; i < 5; i++) {
		if (wordle_state->secret_word[i] == local_buf[i])
		    wordle_state->feed_back[i] = '2';
		else if (wordle_contains(local_buf[i]))
		    wordle_state->feed_back[i] = '1';
		else
		    wordle_state->feed_back[i] = '0';
	    }
	}

	return len;
}

static int wordle_open (struct inode *i, struct file *f) {
	f->f_op = &wordle_file_operations;
	f->private_data = wordle_state;

	// resets everything, don't need here
	// wordle_state_reset();

	return 0;
}


/*** wordle module setup ***/

/* called on module initialization */
static int __init wordle_module_init(void)
{
	printk (KERN_INFO "wordle: initializing\n");

	int err =
		misc_register(&wordle_miscdevice);

	if (err != 0) {
		printk("wordle_module_init: misc_register error\n");
		return err;
	}


	wordle_state = (struct WordleState*) kzalloc(sizeof(struct WordleState), GFP_KERNEL);
	if (wordle_state == NULL) {
		printk("wordle_module_init: kzalloc error\n");
		return ENOMEM;
	}

	wordle_state_reset();


	printk (KERN_INFO "wordle: initialization complete\n");
	return 0;
}

/* called on module exit */
static void __exit wordle_module_exit(void)
{
	kfree(wordle_state);

	misc_deregister(&wordle_miscdevice);
	printk(KERN_INFO "wordle: exiting\n");
}


module_init (wordle_module_init);
module_exit (wordle_module_exit);

MODULE_ALIAS_MISCDEV(MISC_DYNAMIC_MINOR);
MODULE_LICENSE     ("GPL");
MODULE_DESCRIPTION ("wordle character device game");
MODULE_AUTHOR      ("nicholas_calabro");


/*** wordle helper definitions ***/

static void wordle_state_reset(void) {
	wordle_state->secret_word = wordle_secret_word();
	wordle_state->game_status = GS_IN_PROGRESS;
	wordle_state->guess_count = 5;
	wordle_state->feed_back[0] = '0';
	wordle_state->feed_back[1] = '0';
	wordle_state->feed_back[2] = '0';
	wordle_state->feed_back[3] = '0';
	wordle_state->feed_back[4] = '0';
}

static char* wordle_secret_word() {
	static int total_secrets = sizeof(wordle_words) / sizeof(char*);
	u32 r = get_random_u32();

	if (total_secrets < 100)
		printk (KERN_INFO "wordle_secret_word: warning; total_secrets is not high enough to pass specification standards\n");

	return wordle_words[r % total_secrets];
}

static int wordle_strlen(const char *s) {
	if (!s) return -1;

	int n = 0;
	while (s[n] != '\0')
		n++;

	return n;
}

static int wordle_isupper(char c) {
	return c <= 'Z' && c >= 'A';
}

static int wordle_contains(char c) {
	for (int i = 0; i < 5; i++)
		if (wordle_state->secret_word[i] == c)
			return 1;

	return 0;
}

static void wordle_strncpy(char *dest, char *src, int n) {
	for (int i = 0; i < n; i ++)
		dest[i] = src[i];
}

static int wordle_strncmp(char *str1, char *str2, int n) {
	for (int i = 0; i < n; i++)
		if (str1[i] != str2[i])
			return 1;
	return 0;
}

