#include <stdio.h>
#include <time.h>

unsigned ncpus = 0;

unsigned cpu_count()
{
    char line[500];
    unsigned n = 0;
    FILE *f = fopen("/proc/stat", "r");
    fgets(line, sizeof line, f);
    fgets(line, sizeof line, f);
    while(line[0]=='c' && line[1]=='p' && line[2]=='u') {
        fgets(line, sizeof line, f);
        n++;
    }
    fclose(f);
    return n;
}

long long unsigned for_all()
{
    long long unsigned user, nice, system, idle, irq, softirq, steal, guest;
    FILE *f = fopen("/proc/stat", "r");
    fscanf(f, "cpu %llu %llu %llu %llu %llu %llu %llu %llu", &user, &nice, &system, &idle, &irq, &softirq, &steal, &guest);
    fclose(f);
    return user + nice + system + idle + irq + softirq + steal + guest;
}

long long unsigned for_me()
{
    long long unsigned utime, stime;
    FILE *f = fopen("/proc/self/stat", "r");
    fscanf(f, "%*u %*s %*c %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %llu %llu", &utime, &stime);
    fclose(f);
    return utime + stime;
}

int main()
{
    time_t t0, tf;
    long unsigned a, b, c, r, n;
    long long unsigned fabeg, faend;

    fabeg = for_all();

    t0 = tf = time(NULL);
    do {
        for (a = 0 ; a < 100 ; a++)
            for (b = 0 ; b < 1000 ; b++)
                for (c = 0 ; c < 1000 ; c++)
                    ;
        tf = time(NULL);
    }
    while(tf - t0 < 10); /* 10 seconds */

    faend = for_all();
    r = (100 * for_me() * cpu_count()) / (faend - fabeg);
    printf("IGNORE(%lld %%)\n", r);
    if (14 <= r && r <= 16)
        printf("OK\n");
    else
        printf("NOT OK\n");

    return 0;
}
