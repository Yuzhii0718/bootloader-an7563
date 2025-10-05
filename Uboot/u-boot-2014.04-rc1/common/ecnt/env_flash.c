#include <common.h>
#include <environment.h>
#include <malloc.h>
#include <search.h>
#include <errno.h>
#include <flashhal.h>
#include <ecnt/en7523_evb_mtk_env_parser.h>

DECLARE_GLOBAL_DATA_PTR;

char *env_name_spec = "ECNT Flash";

int saveenv(void)
{
	env_t			env_new;
	ssize_t			len = 0;
	char			*res = NULL;
	unsigned long	retlen = 0;

	res = (char *) &env_new.data;
	len = hexport_r(&env_htab, '\0', 0, &res, ENV_SIZE, 0, NULL);
	if (len < 0)
	{
		error("Cannot export environment: errno = %d\n", errno);
		return 1;
	}
	env_new.crc = crc32(0, env_new.data, ENV_SIZE);


	if (flash_erase(CONFIG_ENV_MTK_OFFSET, CONFIG_ENV_SIZE))
	{
		return 1;
	}

	if (flash_write(CONFIG_ENV_MTK_OFFSET, CONFIG_ENV_SIZE, &retlen, (unsigned char *) &env_new))
	{
		return 1;
	}

	printf("[MTK] Save env to 0x%x\n", CONFIG_ENV_MTK_OFFSET);

	return 0;
}


int mtk_save_miconf(void)
{
	unsigned long retlen = 0;
	char *res = NULL;
	ssize_t len = 0;
	env_t env_new;

	res = (char *) &env_new.data;
	len = hexport_r(&env_htab, '\0', 0, &res, ENV_SIZE, 0, NULL);
	if (len < 0)
	{
		error("Cannot export environment: errno = %d\n", errno);
		return 1;
	}
	env_new.crc = crc32(0, env_new.data, ENV_SIZE);

	if (flash_erase_non_block(CONFIG_ENV_AIROHA_OFFSET, CONFIG_ENV_SIZE))
	{
		return 1;
	}

	if (flash_write(CONFIG_ENV_AIROHA_OFFSET, CONFIG_ENV_SIZE, &retlen, (unsigned char *) &env_new))
	{
		return 1;
	}

	printf("[MTK] Save env to 0x%x\n", CONFIG_ENV_AIROHA_OFFSET);

	return 0;
}

int mtk_check_first_boot(char* sig)
{
	unsigned int addr = CONFIG_ENV_AIROHA_OFFSET + 0x4;
	unsigned int read_len = ENV_READ_LEN - 1;
	uint8_t buf[ENV_READ_LEN];
	char config[ENV_READ_LEN];
	unsigned long retlen;
	int found = 0;
	int ret = 0;

	while (addr < CONFIG_ENV_MTK_OFFSET) {
		/* Check addr boundary */
		if (addr + read_len > CONFIG_ENV_MTK_OFFSET)
			read_len = CONFIG_ENV_MTK_OFFSET - addr;

		/* Read miconf to buf */
		ret = flash_read(addr, read_len, retlen, (uint8_t *) buf);
		if (ret)
			goto out;

		/* Transfer '\0' to ';' for parsing */
		mtk_transfer_config(buf, read_len);

		/* Find signature from buf */
		if (env_get_from_linear(buf, sig, config)) {
			found = 1;
			break;
		}

		addr += ENV_READ_LEN;
	}

	/* Parse MTK_MOVE_ENV from char to int */
	if (found)
		ret = mtk_parse_config_to_bool(config);

	return ret;

out:
	if (buf)
		free(buf);

	return ret;
}

void mtk_move_miconf(void)
{
	/* Move mi.conf from 0x7C000 to 0x80000 */
	unsigned long retlen;
	char *buf;
	int ret;

	printf("[MTK] Copying mi.conf from 0x%x to 0x%x\n", CONFIG_ENV_AIROHA_OFFSET,
	       CONFIG_ENV_MTK_OFFSET);

	buf = (char *)malloc(CONFIG_ENV_SIZE);

	ret = flash_read(CONFIG_ENV_AIROHA_OFFSET,
                     CONFIG_ENV_SIZE, &retlen, (unsigned char *) buf);
	if (ret)
		goto out;

	if (flash_erase(CONFIG_ENV_MTK_OFFSET, CONFIG_ENV_SIZE))
	{
		return 1;
	}

	ret = flash_write(CONFIG_ENV_MTK_OFFSET, CONFIG_ENV_SIZE, &retlen, (unsigned char *) buf);

	if (ret)
		goto out;

out:
	if (buf)
		free(buf);
}

void disable_mtk_move_env(void)
{
	unsigned long retlen;
	char *buf = NULL;
	int ret;

	buf = (char *) malloc(CONFIG_ENV_SIZE);

	ret = flash_read(CONFIG_ENV_AIROHA_OFFSET, CONFIG_ENV_SIZE, &retlen, (unsigned char *) buf);
	if (ret) {
		set_default_env("!flash_read() failed");
		goto out;
	}

	ret = env_import(buf, 1);
	if (ret)
		gd->env_valid = 1;

	ret = setenv("MTK_MOVE_ENV", ENV_MOVE_DISABLE);
	if (ret)
		goto out;

	ret = mtk_save_miconf();
	if (ret)
		goto out;

out:
	if (buf)
		free(buf);
}

void env_relocate_spec(void)
{
	unsigned int env_addr = CONFIG_ENV_MTK_OFFSET;
	unsigned long retlen;
	char *buf = NULL;
	int ret = 0;

	/*
	 * Check whether it is first boot or not (MTK_MOVE_ENV)
	 * If yes, it's necessary to
	 *	(1) disable MTK_MOVE_ENV and re-save to 0x7C000
	 *	(2) copy mi.conf from 0x7C000 to 0x80000
	 * Customer will always operate env addr with 0x80000 (u-boot-env)
 	 */
	ret = mtk_check_first_boot("MTK_MOVE_ENV");
	if (ret) {
		printf("[MTK] MTK_MOVE_ENV is enabled!\n");
		disable_mtk_move_env();
		mtk_move_miconf();
	}

	buf = (char *) malloc(CONFIG_ENV_SIZE);

	ret = flash_read(env_addr, CONFIG_ENV_SIZE, &retlen, (unsigned char *) buf);
	if (ret) {
		set_default_env("!flash_read() failed");
		goto out;
	}

	ret = env_import(buf, 1);
	if (ret)
		gd->env_valid = 1;

out:
	if (buf)
		free(buf);
}

int env_init(void)
{
	/* SPI flash isn't usable before relocation */
	gd->env_addr = (ulong)&default_environment[0];
	gd->env_valid = 1;

	return 0;
}

