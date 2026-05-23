#include <hardware/i2c.h>

void setup_shared_i2c(void);
i2c_inst_t* get_shared_i2c_instance(void);

void setup_puppet_i2c_as_shared_i2c(void);
i2c_inst_t* get_puppet_shared_i2c_instance(void);

void i2c_scan(i2c_inst_t *i2c);
