/* dummy phys_io */


void da_initialize(void)
{
	printf("da_initialize\n");
}

void da_write(int value)
{
}

void ad_initialize(void)
{
	printf("ad_initialize\n");
}

void ad_read(unsigned int *ch0data, unsigned int *ch1data)
{

	*ch0data = 0;
	*ch1data = 0;


}
