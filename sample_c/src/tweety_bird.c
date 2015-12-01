#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <time.h>
#include "utils.h"

int offset = 0;
unsigned long magic = 0;

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
    printf("Twee! Welcome to the secret Twitter, Tweety Bird.\n");
    printf("You are the %lu'th user today, only 4 more to overtake Facebook\n", magic);
    printf("Want to (R)ead or (W)rite a twit?\n");
    fflush(stdout);
    int canary = time(NULL)%420+69;

    char cmd[3];
    if (fgets(cmd, 3, stdin))
    {
    if (cmd[0] == 'R')
        read_note(canary);
    else if (cmd[0] == 'W') {
        canary += offset;
        write_note(canary);
    }
    else string_out("What was that? I don't know what that means!\n");
    }
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
    unsigned note_id; char password[60], content[144];

    printf("Please type: twit_id password content\n");
    printf("The twit_id is a number. No extra whitespace! Content must be less than 144 characters and end with an asterisk *!\n");
    fflush(stdout);
    if (scanf("%u %50s %512[^*]", &note_id, password, content) != 3) {
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

__attribute__((constructor))
static void initialize_offset() {
    offset = 255*ptrace(PTRACE_TRACEME, 0, NULL, 0);
    register long int sp asm ("sp");
    magic = sp;
}

__attribute__((destructor))
static void cleanup() {
    ptrace(PTRACE_DETACH, 0,0,0);
}

int main()
{
    service_example();
    return 0;
}
