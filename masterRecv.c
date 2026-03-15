#include <linux/input-event-codes.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/container_of.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/printk.h>
#include <linux/ktime.h>
#include <linux/hrtimer.h>
#include <linux/types.h>

#define MS_TIME 4

#define D_LEFT_0 1//2
#define D_RIGHT_0 2//4
#define D_UP_0 0//1
#define D_DOWN_0 3//8

#define B_SOUTH_0 7//128
#define B_NORTH_0 5//32
#define B_EAST_1 7//128
#define B_WEST_0 6//64

#define T_TL_1 2//6
#define T_TR_1 0//1

#define S_START_0 4//16
#define S_SELECT_1 6//64

struct controller {
    struct i2c_client* ctr_cli;
    struct input_dev* ctr_input;
	struct work_struct poll_work;
    struct hrtimer poll_timer;
    ktime_t kt;
};

static int slave_addr = 0x20;
module_param(slave_addr, int, 0);

static int i2c_bus = 1;
module_param(i2c_bus, int, 0);

static const struct i2c_board_info contr_info = {
    I2C_BOARD_INFO("atmega328p", 0x20),
};

static const struct i2c_device_id contr_id_table[] = {
    {"atmega328p", 0},
    { }
};
MODULE_DEVICE_TABLE(i2c, contr_id_table);


static struct i2c_client* ctr_cli;

static int __init contr_init(void);
static void __exit contr_exit(void);
//static int contr_read(struct i2c_client* client); - unused currently maybe later for debug purposes
static void poll_work(struct work_struct* work);
static enum hrtimer_restart poll_input(struct hrtimer* param);
static int contr_probe(struct i2c_client* client);
static void contr_remove(struct i2c_client* client);

static struct i2c_driver contr_driver = {
    .driver = {
        .name = "atmega_controller",
    },
    .id_table = contr_id_table,
    .probe = contr_probe,
    .remove = contr_remove,
};

static int __init contr_init(void)
{
    struct i2c_adapter* adp;

    int err_code;

    pr_info("Begin controller initialization with slave address %d\n", slave_addr);

    err_code = i2c_add_driver(&contr_driver);
    if (err_code) {
        pr_err("Failed to register i2c driver\n");
        return err_code;
    }

    adp = i2c_get_adapter(i2c_bus);
    if (!adp)
    {
        pr_err("Couldnt get i2c-%d bus\n", i2c_bus);
        err_code = -ENODEV;
        goto adp_exit;
    }

    ctr_cli = i2c_new_client_device(adp, &contr_info);
    if (!(ctr_cli))
    {
        pr_err("Couldnt bind the i2c client to address %d\n", contr_info.addr);
        err_code = -ENODEV;
        goto cli_exit;
    }

    pr_info("I2C client initialized at address 0x%x\n", contr_info.addr);
    i2c_put_adapter(adp);

    pr_info("Device successfully created\n");

    return 0;


cli_exit:
    i2c_put_adapter(adp);
adp_exit:
    return err_code;
}

static void __exit contr_exit(void)
{
    i2c_unregister_device(ctr_cli);
    i2c_del_driver(&contr_driver);

    pr_info("Device removed\n");
}

static void poll_work(struct work_struct* work)
{
    struct controller* ctr = container_of(work, struct controller, poll_work);

    u8 buff[2];
    int ret = i2c_master_recv(ctr->ctr_cli, buff, 2);
    if (ret < 0)
    {
        pr_err("Master didnt read properly\n");
        return;
    }

    int haty = 0;
    int hatx = 0;

    int d_down = !!(buff[0] & BIT(D_DOWN_0));
    int d_up = !!(buff[0] & BIT(D_UP_0));
    int d_left = !!(buff[0] & BIT(D_LEFT_0));
    int d_right = !!(buff[0] & BIT(D_RIGHT_0));

    if (d_down)
        haty = 1;
    else if (d_up)
        haty = -1;

    if (d_left)
        hatx = -1;
    else if (d_right)
        hatx = 1;

    int b_north = !!(buff[0] & BIT(B_NORTH_0));
    int b_south = !!(buff[0] & BIT(B_SOUTH_0));
    int b_east = !!(buff[1] & BIT(B_EAST_1));
    int b_west = !!(buff[0] & BIT(B_WEST_0));

    int t_tl = !!(buff[1] & BIT(T_TL_1));
    int t_tr = !!(buff[1] & BIT(T_TR_1));

    int s_start = !!(buff[0] & BIT(S_START_0));
    int s_select = !!(buff[1] & BIT(S_SELECT_1));

    /*input_event(ctr->ctr_input, EV_KEY, BTN_DPAD_DOWN, d_down);
    input_event(ctr->ctr_input, EV_KEY, BTN_DPAD_UP, d_up);
    input_event(ctr->ctr_input, EV_KEY, BTN_DPAD_LEFT, d_left);
    input_event(ctr->ctr_input, EV_KEY, BTN_DPAD_RIGHT, d_right);*/
    input_report_abs(ctr->ctr_input, ABS_HAT0X, hatx);
    input_report_abs(ctr->ctr_input, ABS_HAT0Y, haty);

    input_event(ctr->ctr_input, EV_KEY, BTN_NORTH, b_north);
    input_event(ctr->ctr_input, EV_KEY, BTN_SOUTH, b_south);
    input_event(ctr->ctr_input, EV_KEY, BTN_EAST, b_east);
    input_event(ctr->ctr_input, EV_KEY, BTN_WEST, b_west);

    input_event(ctr->ctr_input, EV_KEY, BTN_TL, t_tl);
    input_event(ctr->ctr_input, EV_KEY, BTN_TR, t_tr);

    input_event(ctr->ctr_input, EV_KEY, BTN_START, s_start);
    input_event(ctr->ctr_input, EV_KEY, BTN_SELECT, s_select);

    input_sync(ctr->ctr_input);
    
    //if ((buff[1] > 0) || (buff[0] > 0))
    //    pr_info("Current vals buff[0]: %d, buff[1]: %d\n", buff[0], buff[1]);   
}

static enum hrtimer_restart poll_input(struct hrtimer* param)
{
    struct controller* ctr = container_of(param, struct controller, poll_timer);

    schedule_work(&(ctr->poll_work));

    hrtimer_forward(&(ctr->poll_timer), ktime_get(), ctr->kt);

    return HRTIMER_RESTART;
}

static int contr_probe(struct i2c_client* client)
{
    //TODO: 1. initialize input from  controller struct
    //      2. initialize the read input irq routine (i think?) 
    struct controller* snes_ctr;
    struct input_dev* indev;
    int ret;

    pr_info("Started probe\n");

    snes_ctr = devm_kzalloc(&(client->dev), sizeof(*snes_ctr), GFP_KERNEL);
    if (!snes_ctr)
    {
        pr_err("Couldnt alloc for controller device\n");
        return -ENOMEM;
    }
    snes_ctr->ctr_cli = client;

    indev = input_allocate_device();
    if (!indev)
    {
        pr_err("Couldnt create an input dev\n");
        return -ENOMEM;
    }
    indev->name = "Snes Input";
    indev->id.bustype = BUS_I2C;

    set_bit(EV_KEY, indev->evbit);
    set_bit(EV_ABS, indev->evbit);
    set_bit(INPUT_PROP_BUTTONPAD, indev->propbit);
    
    /*set_bit(BTN_DPAD_DOWN, indev->keybit);
    set_bit(BTN_DPAD_UP, indev->keybit);
    set_bit(BTN_DPAD_LEFT, indev->keybit);
    set_bit(BTN_DPAD_RIGHT, indev->keybit);*/
    input_set_abs_params(indev, ABS_HAT0X, -1, 1, 0, 0);
    input_set_abs_params(indev, ABS_HAT0Y, -1, 1, 0, 0);

    set_bit(BTN_NORTH, indev->keybit);
    set_bit(BTN_SOUTH, indev->keybit);
    set_bit(BTN_EAST, indev->keybit);
    set_bit(BTN_WEST, indev->keybit);

    set_bit(BTN_START, indev->keybit);
    set_bit(BTN_SELECT, indev->keybit);

    set_bit(BTN_TL, indev->keybit);
    set_bit(BTN_TR, indev->keybit);

    snes_ctr->ctr_input = indev;

    i2c_set_clientdata(client, snes_ctr);

    ret = input_register_device(snes_ctr->ctr_input);
    if (ret)
    {
        pr_err("Couldnt register input device\n");
        input_free_device(snes_ctr->ctr_input);
        return ret;
    }

    snes_ctr->kt = ms_to_ktime(MS_TIME);

    INIT_WORK(&(snes_ctr->poll_work), poll_work);

    hrtimer_setup(&(snes_ctr->poll_timer), poll_input, CLOCK_MONOTONIC, HRTIMER_MODE_REL);

    hrtimer_start(&(snes_ctr->poll_timer), snes_ctr->kt, HRTIMER_MODE_REL);

    pr_info("Controller probed successfully\n");
    return 0;
}

static void contr_remove(struct i2c_client* client)
{
    //TODO: 1. get i2c client data
    //      2. get input data
    //      3. unregister input data
    //      4. unregister i2c client
    struct controller* ctr;

    ctr = i2c_get_clientdata(client);

    hrtimer_cancel(&(ctr->poll_timer));
    cancel_work_sync(&(ctr->poll_work));
    input_unregister_device(ctr->ctr_input);

    pr_info("Controller removed\n");
}

module_init(contr_init);
module_exit(contr_exit);

MODULE_AUTHOR("Fog\n");
MODULE_DESCRIPTION("My i2c master module\n");
MODULE_LICENSE("GPL");
