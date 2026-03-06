#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/printk.h>

struct controller {
    struct i2c_client* ctr_cli;
    struct input_dev* ctr_input;
};

static int slave_addr = 0x20;
module_param(slave_addr, int, 0);

static int i2c_bus = 0;
module_param(i2c_bus, int, 0);

static const struct i2c_board_info contr_info = {
    I2C_BOARD_INFO("atmega328p", slave_addr),
};

static const struct i2c_device_id contr_id_table[] = {
    {"atmega328p", 0},
    { }
};
MODULE_DEVICE_TABLE(i2c, contr_id_table);

static int __init contr_init(void);
static void __exit contr_exit(void);
static int contr_read(struct i2c_client* client);
static int contr_probe(struct i2c_client* client);
static void contr_remove(struct i2c_client* client);


static int __init contr_init(void)
{
    struct controller* atm_ctr;
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

    atm_ctr = devm_kzalloc(&(adp->dev), sizeof(*atm_ctr), GFP_KERNEL);
    if (!atm_ctr)
    {
        pr_err("Couldnt allocate memory for device\n");
        err_code = ENOMEM;
        goto dev_exit;
    }

    atm_ctr->ctr_cli = i2c_new_client_device(adp, contr_info);
    if (!(atm_ctr->ctr_cli))
    {
        pr_err("Couldnt bind the i2c client to address %d\n", contr_info.addr);
        err_code = ENODEV;
        goto cli_exit;
    }

    pr_info("I2C client initialized at address 0x%x\n", contr_info.addr);
    return 0;

cli_exit:
dev_exit:
    i2c_put_adapter(adp);
adp_exit:
    return -err_code;
}

static void __exit contr_exit(void)
{
    pr_info("Device removed\n");
}

static int contr_probe(struct i2c_client* client)
{
    //TODO: 1. initialize input from  controller struct
    //      2. initialize the read input irq routine (i think?) 

    return 0;
}

static void contr_remove(struct i2c_client* client)
{
    //TODO: 1. get i2c client data
    //      2. get input data
    //      3. unregister input data
    //      4. unregister i2c client
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
