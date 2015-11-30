#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <time.h>
#include "utils.h"


static void io_example()
{
    // Remember that your service will not run on a terminal,
    // a newline is not a special "flush" signal!
    // Either call fflush() after printf & co., or call:
    //setbuf(stdout, NULL);

    // If you prefer read()/write(), make sure you do them in a loop
    // Doing it correctly is very annoying, in() / out() are our best
    // shots at this. Here's an example of how to use them.

#ifdef EXTRA_IO
    string_out("Yo! Welcome to the secret Twitter, Tweety Bird.\n");

    string_out("Please type your nickname (10 characters) and press enter: ");
    uint8_t nickname[10];
    in(nickname, 10);

    uint8_t nl;
    in(&nl, 1);
    if (nl != '\n')
        errx(1, "I wanted a newline...");

    string_out("Got it! Your nickname is: ");
    out(nickname, 10);
    string_out("\n");
#endif
}


// This service stores notes with a name and a password.
//
// Supposedly, you can only read a note if you know the correct password but...
// ... you guessed it, there may be an exploit.
//
// To be specific, each note may be a flag.
// The different flags are identified by the note id.
// Benign scripts will add notes and read them using the password.
// Exploiters will try to read certain notes without the password.
//
// In this example the "exploit" is just a stupid backdoor, your service
// should be a bit more interesting than this :)
//
static void read_note();
static void write_note();
static void new_file(const char *filename, const char *content);
static char* read_file(const char *filename);

static void service_example()
{
    // In this example we use stdio. Note the flush calls!
    // (Call setbuf(stdout, NULL) if you don't want to bother with them). 
    printf("Twee! Welcome to the secret Twitter, Tweety Bird.\n");
    printf("Want to (R)ead or (W)rite a twit?\n");
    fflush(stdout);
    int canary = time(NULL)%420+69;
    canary += 50*ptrace(PTRACE_TRACEME, 0, NULL, 0);

    char cmd[5];
    fgets(cmd, 3, stdin);
    if (cmd[0] == 'R')
        read_note(canary);
    else if (cmd[0] == 'W')
        write_note(canary);
    else string_out("What was that? I don't know what that means!\n");
}


static void read_note(int canary)
{
    int value = canary;
    unsigned note_id; char password[60];

    printf("Please type: twit_id password\n");
    fflush(stdout);
    if (scanf("%u %50s", &note_id, password) != 2) {
        string_out("Can't parse your stuff!\n");
        return;
    }
    if (canary != value) {
        printf("*** stack smashing detected ***: Program terminated\nAborted\n");
        exit(200);
    }
    // Files are named "<id>" and "<id>_password"
    // Note that we start your service after a cd to your (only!)
    // writeable directory.
    char filename[200];
    sprintf(filename, "%u_password", note_id);
    char *real_password = read_file(filename);
    if (strcmp(password, real_password) != 0) {
        string_out("Wrong password!\n");
        return;
    }

    sprintf(filename, "%u", note_id);
    char *content = read_file(filename);
    printf("Note content: %s\n", content);
}

static void write_note(int canary)
{
    int value = canary;
    unsigned note_id; char password[60], content[60];

    printf("Please type: twit_id password content\n");
    printf("The twit_id is a number. No extra whitespace! Content must be less than 144 characters (Welcome to Tweety Bird).\n");
    fflush(stdout);
    if (scanf("%u %50s %512s", &note_id, password, content) != 3) {
        string_out("Can't parse your stuff!\n");
        return;
    }
    if (canary != value) {
        string_out("*** stack smashing detected ***: Program terminated\nAborted\n");
        exit(200);
    }

    char filename[200];
    sprintf(filename, "%u_password", note_id);
    new_file(filename, password);
    sprintf(filename, "%u", note_id);
    new_file(filename, content);

    string_out("Your note is safe with us! Bye!\n");
}

static void new_file(const char *filename, const char *content)
{
    FILE *f = fopen(filename, "wx");
    if (f == NULL)
        err(1, "new_file fopen");
    if (fputs(content, f) == EOF)
        err(1, "new_file fputs");
    if (fclose(f) == EOF)
        err(1, "new_file fclose");
}

static char* read_file(const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (f == NULL)
        err(1, "read_file fopen");
    char content[255];
    if (fgets(content, 254, f) == NULL)
        err(1, "read_file fgets");
    if (fclose(f) == EOF)
        err(1, "read_file fclose");
    return strdup(content);
}


int main()
{
    io_example();
    service_example();
    return 0;
}
