#ifndef SETTINGS_H
#define SETTINGS_H
typedef struct {
	char type; // 0 = int, 1 = string, 2 = double
	char *name;
	void *data;
	bool def;
} SettingsEntry;

int settingssize = 0;
SettingsEntry *settings = NULL;
void changeSetting(char * name,void *data)
{
	if (settings == NULL)
	{
		return;
	}
	for (int i = 0;i<settingssize;i++)
	{
		if (strcmp(name,settings[i].name) == 0)
		{
			if (settings[i].def)
			{
				settings[i].data = data;
			}
			else
			{
				if (settings[i].type == 1)
				{
					free(settings[i].data)
				}
				settings[i].data = data;
			}
			return;
		}
	}
}
void loadsettings()
{
	File *settingsf = fopen("/OSExt config.conf","r");
	if (settingsf == NULL)
	{
		defaultsettings();
		return;
	}
	int matches;
	int length;
	matches = fscanf(settingsf,"length: %d\n",&length)
	if (matches == 0)
	{
		fclose(settingsf);
		defaultsettings();
		return;
	}
	settingssize = length;
	settings = calloc(length,sizeof(SettingsEntry));
	for (int i = 0;i<length;i++)
	{
		char type = 0;
		matches = fscanf(settingsf,"%c ",&type);
		if (matches == 0)
		{
			free(settings);
			settings = NULL;
			fclose(settingsf);
			defaultsettings();
			return;
		}
		if (type < 0 || type > 2)
		{
			free(settings);
			settings = NULL;
			fclose(settingsf);
			defaultsettings();
			return;
		}
		char *name = calloc(11,sizeof(char));
		matches = fscanf(settingsf,"%10s ");
		if (matches == 0)
		{
			free(settings);
			settings = NULL;
			fclose(settingsf);
			defaultsettings();
			return;
		}
		if (d == 0)
		{
			int *data = calloc(1,sizeof(int));
			matches = fscanf(settingsf,"%d\n",data);
			if (matches == 0)
			{
				free(data);
				free(settings);
				settings = NULL;
				fclose(settingsf);
				defaultsettings();
				return;
			}
			settings[i].name = name;
			settings[i].type = type;
			settigns[i].data = data;
		}
		if (d == 1)
		{
			int *data = calloc(31,sizeof(char));
			matches = fscanf(settingsf,"%30s\n",data);
			if (matches == 0)
			{
				free(data);
				free(settings);
				settings = NULL;
				fclose(settingsf);
				defaultsettings();
				return;
			}
			settings[i].name = name;
			settings[i].type = type;
			settigns[i].data = data;
		}
		if (d == 2)
		{
			int *data = calloc(1,sizeof(double));
			matches = fscanf(settingsf,"%f\n",data);
			if (matches == 0)
			{
				free(data);
				free(settings);
				settings = NULL;
				fclose(settingsf);
				defaultsettings();
				return;
			}
			settings[i].name = name;
			settings[i].type = type;
			settigns[i].data = data;
		}
		settings[i].def = false;
	}
	fclose(settingsf);
}
void savesettings()
{
	File *settingsf = fopen("/OSExt config.conf","w");
	if (settingsf == NULL)
	{
		return;
	}
	fprintf(settingsf,"length: %d\n",settingssize)
	for (int i = 0;i<settingssize;i++)
	{
		fprintf(settingsf,"%c ",settings[i].type);
		fprintf(settingsf,"%10s ",settings[i].name);
		if (settings[i].type == 0)
		{
			fprintf(settingsf,"%d\n",*(settings[i].data));
		}
		if (settings[i].type == 1)
		{
			fprintf(settingsf,"%30s\n",*(settings[i].data));
		}
		if (settings[i].type == 2)
		{
			fprintf(settingsf,"%f\n",*(settings[i].data));
		}
	}
	fclose(settingsf);
}
void defaultSettings()
{
	int length = 0;
	#ifdef MODULE_DESKTOP
	length += 3;
	#endif
	
	
	settings = calloc(length,sizeof(SettingsEntry));
	int index = 0;
	
	
	#ifdef MODULE_DESKTOP
	settings[index].type = 0;
	settings[index].name = "r";
	settings[index].def = true;
	settings[index].data = malloc(sizeof(int))
	*(settings[index].data) = 255;
	index++;
	settings[index].type = 0;
	settings[index].name = "g";
	settings[index].def = true;
	settings[index].data = malloc(sizeof(int))
	*(settings[index].data) = 255;
	index++;
	settings[index].type = 0;
	settings[index].name = "b";
	settings[index].def = true;
	settings[index].data = malloc(sizeof(int))
	*(settings[index].data) = 255;
	index++;
	#endif
	
	
	
	
	
	
	
	
	
	
	
	
	
}



#endif