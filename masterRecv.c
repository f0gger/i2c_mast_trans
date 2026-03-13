#include <linux/input-event-codes.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/container_of.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/printk.h>

struct controller {
    struct i2c_client* ctr_cli;
    struct input_dev* ctr_input;
    struct delayed_work dwork;
};

static int slave_addr = 0x20;
module_param(slave_addr, int, 0);

static int i2c_bus = 0;
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
static void poll_input(struct work_struct* work);
static int contr_probe(struct i2c_client* client);
static void contr_remove(struct i2c_client* client);

static int __init contr_init(void)
{
    struct i2c_adapter* adp;

    int err_code;

    pr_info("Begin controller initialization with slave address %d\n", slave_addr);

    adp = i2c_get_adapter(i2c_bus);
    if (!adp)
    {
        pr_err("Couldnt get i2c-%d bus\n", i2c_bus);
        err_code = ENODEV;
        goto adp_exit;
    }

    ctr_cli = i2c_new_client_device(adp, &contr_info);
    if (!(ctr_cli))
    {
        pr_err("Couldnt bind the i2c client to address %d\n", contr_info.addr);
        err_code = ENODEV;
        goto cli_exit;
    }

    pr_info("I2C client initialized at address 0x%x\n", contr_info.addr);
    i2c_put_adapter(adp);

    pr_info("Device successfully created\n");

    return 0;


cli_exit:
    i2c_put_adapter(adp);
adp_exit:
    return -err_code;
}

static void __exit contr_exit(void)
{
    i2c_unregister_device(ctr_cli);
    pr_info("Device removed\n");
}

static void poll_input(struct work_struct* work)
{
    struct controller* ctr = container_of(work, struct controller, dwork.work);

    int val = i2c_smbus_read_byte(ctr->ctr_cli);
    pr_info("Current val: %d\n", val);

    schedule_delayed_work(&(ctr->dwork), msecs_to_jiffies(4));
}

static int contr_probe(struct i2c_client* client)
{
    //TODO: 1. initialize input from  controller struct
    //      2. initialize the read input irq routine (i think?) 
    struct controller* snes_ctr;
    struct input_dev* indev;
    int ret;

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
    set_bit(BTN_DPAD_DOWN, indev->keybit);
    set_bit(BTN_DPAD_UP, indev->keybit);
    set_bit(BTN_DPAD_LEFT, indev->keybit);
    set_bit(BTN_DPAD_RIGHT, indev->keybit);
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
        return ret;
    }

    INIT_DELAYED_WORK(&(snes_ctr->dwork), poll_input);
    queue_delayed_work(system_wq, &(snes_ctr->dwork), msecs_to_jiffies(4));

    pr_info("Constroller probed successfully\n");
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

    cancel_delayed_work_sync(&(ctr->dwork));
    input_unregister_device(ctr->ctr_input);

    pr_info("Controller removed\n");
}

static struct i2c_driver contr_driver = {
    .driver = {
        .name = "atmega contoller",
    },
    .id_table = contr_id_table,
    .probe = contr_probe,
    .remove = contr_remove,
};

module_init(contr_init);
module_exit(contr_exit);

MODULE_AUTHOR("Fog\n");
MODULE_DESCRIPTION("My i2c master module\n");
MODULE_LICENSE("GPL");
