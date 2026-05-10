#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

pthread_t start_rt_thread(const char *name, void *(*func)(void *), void *arg, size_t stackSize, int priority);
void pabort(const char *msg);

#define MAIN_PRIO 70

bool do_terminate = false;

void on_sigint(int signum)
{
    printf("!!! SIGINT handler\n");
    do_terminate = true;
}

static gpiod_chip *gpio_chip;
static gpiod_line *gpio0_a4;
static gpiod_line_event event;

void *test_loop(void *)
{
    gpio_chip = gpiod_chip_open("/dev/gpiochip0");
    gpio0_a4 = gpiod_chip_get_line(gpio_chip, 4);
    int res = gpiod_line_request_rising_edge_events(gpio0_a4, "GPIO-A4");
    if(res < 0)
        pabort("gpiod_line_request_rising_edge_events(4)");

    gpiod_line *gpio0_b5 = gpiod_chip_get_line(gpio_chip, 13);
    gpiod_line_request_output(gpio0_b5, "user", 0);

    for(int i = 0; !do_terminate; i++) {
        gpiod_line_event_wait(gpio0_a4, nullptr);
        gpiod_line_event_read(gpio0_a4, &event); // SIGSEGV после return, если event локальный (странно)

        gpiod_line_set_value(gpio0_b5, 1);
        usleep(2000);
        gpiod_line_set_value(gpio0_b5, 0);
    }

    gpiod_line_release(gpio0_a4);

    return nullptr;
}

int main()
{
    setlinebuf(stdout); // для вывода console в qtcreator без fflush

    signal(SIGINT, &on_sigint);

    pthread_t test_thread = start_rt_thread("test", test_loop, nullptr, 100000, MAIN_PRIO);

    while(!do_terminate)
        usleep(300000);

    printf("wait test_thread ...\n");
    pthread_join(test_thread, nullptr);

    return 0;
}

pthread_t start_rt_thread(const char *name, void *(*func)(void *), void *arg, size_t stackSize, int priority)
{
    // Lock memory
    if(mlockall(MCL_CURRENT | MCL_FUTURE) == -1)
        pabort("mlockall");

    // Initialize pthread attributes (default values)
    pthread_attr_t attr;
    int ret = pthread_attr_init(&attr);
    if(ret)
        pabort("pthread_attr_init");

    // Set a specific stack size
    ret = pthread_attr_setstacksize(&attr, stackSize);
    if(ret)
        pabort("pthread_attr_setstacksize");

    // Set scheduler policy and priority of pthread
    ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    if (ret)
        pabort("pthread_attr_setschedpolicy");
    struct sched_param param;
    param.sched_priority = priority;
    ret = pthread_attr_setschedparam(&attr, &param);
    if(ret)
        pabort("pthread_attr_setschedparam");

    // Use scheduling parameters of attr
    ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    if(ret)
        pabort("pthread_attr_setinheritsched");

    pthread_t result;
    ret = pthread_create(&result, &attr, func, arg);
    if(ret)
        pabort("pthread_create");

    pthread_setname_np(result, name);

    return result;
}

void pabort(const char *msg)
{
    printf("%s: %s\n", msg, strerror(errno));
    abort();
}
