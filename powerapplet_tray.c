/*BK battery charge monitor for the notification tray*/
/*(c) Copyright Barry Kauler 2010, bkhome.org*/
/*GPL license: /usr/share/doc/legal/gpl-2.0.txt*/
/*100517 BK only blink icon if not charging...*/
/*101006 BK dir name can be other than BAT0 or BAT1*/
/*110929 BK 2.6.39.4 kernel does not have /proc/acpi/info*/
/*version 2.5 (20120519) rodin.s: added gettext*/
/*version 2.6 (20131215) 01micko change to svg icons*/

#include <stddef.h>
#include <string.h>
#include <libintl.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gstdio.h>
#include <sys/types.h>
#include <dirent.h>

#define _(STRING)    gettext(STRING)

GdkPixbuf *blank_pixbuf;
GdkPixbuf *emptychg_pixbuf;
GdkPixbuf *twentychg_pixbuf;
GdkPixbuf *fortychg_pixbuf;
GdkPixbuf *sixtychg_pixbuf;
GdkPixbuf *eightychg_pixbuf;
GdkPixbuf *fullchg_pixbuf;
GdkPixbuf *emptydis_pixbuf;
GdkPixbuf *twentydis_pixbuf;
GdkPixbuf *fortydis_pixbuf;
GdkPixbuf *sixtydis_pixbuf;
GdkPixbuf *eightydis_pixbuf;
GdkPixbuf *fulldis_pixbuf;

GtkStatusIcon *tray_icon;
unsigned int interval = 5000; /*update interval in milliseconds*/
int batpercent = 100;
int batpercentprev = 0;
int charged;
int pmtype = 0;
FILE *fp;
char inbuf1[200];
char inbuf2[200];
char powerdesign[20]="5500";
int ndesigncapacity=5500;
int nlastfullcapacity=5500;
int charging;
char powerremaining[20];
int npowerremaining;
char memdisplaylong[64];
char batname[16]="";
char batpathinfo[64]="/proc/acpi/battery/";
char batpathstate[64]="/proc/acpi/battery/";
char batpathfallback[64]="/sys/class/power_supply";
char batpathfallbackchargenow[64]="/sys/class/power_supply/";
char batpathfallbackchargefull[64]="/sys/class/power_supply/";
char batpathfallbackstatus[64]="/sys/class/power_supply/";
char batpathfallbackuevent[64]="/sys/class/power_supply/";
char batpathfallbackchargefulldesign[64]="/sys/class/power_supply/";
char batpathfallbackchargenowtmp[64]="/sys/class/power_supply";
char batpathfallbackenergynowtmp[64]="/sys/class/power_supply";

const int PMTYPE_ACPI=2;
const int PMTYPE_APM=1;
const int PMTYPE_POWER_SUPPLY=3;

const int BATT_WARNING=10;
const int BATT_20=20;
const int BATT_40=40;
const int BATT_60=60;
const int BATT_80=80;

GError *gerror = NULL;

gboolean Update(gpointer ptr);

int getACPIStatus();
int getFallbackInt(char[64], int*);
int getFallbackChargeStatus(char[64]);

/**
 * Updates the battery stats.
 * 
 * This reads all the files. 
 */
gboolean Update(gpointer ptr) {
    char strpercent[8];
    char time[8];
    int num;

    charging=1; //charging.
    charged=0; //not full charged.
    
    /*
     * Using acpi
     */
    if (pmtype == PMTYPE_ACPI) {
		if (getACPIStatus() != 0) return;
    }
    /*
     * Using /sys/class/power_supply
     */
    else if (pmtype == PMTYPE_POWER_SUPPLY) {
		// We have to call this multiple times for each variable because of separate files.
        if (getFallbackInt(batpathfallbackchargefulldesign,&ndesigncapacity) != 0) return;
        if (getFallbackInt(batpathfallbackchargefull,&nlastfullcapacity) != 0) return;
		if (getFallbackInt(batpathfallbackchargenow,&npowerremaining) != 0) return;
		if (getFallbackChargeStatus(batpathfallbackstatus) != 0) return;

        //calc percentage charged... check nlastfullcapacity has a sane value...
        if (nlastfullcapacity > 400 && nlastfullcapacity < ndesigncapacity) batpercent=(npowerremaining*100)/nlastfullcapacity;
        else batpercent=(npowerremaining*100)/ndesigncapacity;
        if (charged == 1) batpercent=100; /*101006*/
    }
    else { //apm
        if((fp = fopen("/proc/apm","r")) == NULL) return;
        fscanf(fp,"%*s %*s %*s %*s %*s %*s %7s %d %7s",strpercent,&num,time);
        num = num/(strcmp(time,"sec") == 0?60:1);
        sprintf(time,"%d:%02d",(num/60)%100,num%60);
        fclose(fp);
        batpercent=atoi(strpercent);
    }
    
    //check for mad result...
    if (batpercent < 0) return;
    if (batpercent > 100) return;
    
    //update icon...
    if (gtk_status_icon_get_blinking(tray_icon)==TRUE) gtk_status_icon_set_blinking(tray_icon,FALSE);
    if (charging == 1) {
        if (batpercent <= BATT_WARNING) gtk_status_icon_set_from_pixbuf(tray_icon,emptychg_pixbuf);
        else if (batpercent <= BATT_20) gtk_status_icon_set_from_pixbuf(tray_icon,twentychg_pixbuf);
        else if (batpercent <= BATT_40) gtk_status_icon_set_from_pixbuf(tray_icon,fortychg_pixbuf);
        else if (batpercent <= BATT_60) gtk_status_icon_set_from_pixbuf(tray_icon,sixtychg_pixbuf);
        else if (batpercent <= BATT_80) gtk_status_icon_set_from_pixbuf(tray_icon,eightychg_pixbuf);
        else gtk_status_icon_set_from_pixbuf(tray_icon,fullchg_pixbuf);
    }
    else {
        if (batpercent <= BATT_WARNING) gtk_status_icon_set_from_pixbuf(tray_icon,emptydis_pixbuf);
        else if (batpercent <= BATT_20) gtk_status_icon_set_from_pixbuf(tray_icon,twentydis_pixbuf);
        else if (batpercent <= BATT_40) gtk_status_icon_set_from_pixbuf(tray_icon,fortydis_pixbuf);
        else if (batpercent <= BATT_60) gtk_status_icon_set_from_pixbuf(tray_icon,sixtydis_pixbuf);
        else if (batpercent <= BATT_80) gtk_status_icon_set_from_pixbuf(tray_icon,eightydis_pixbuf);
        else gtk_status_icon_set_from_pixbuf(tray_icon,fulldis_pixbuf);
        /*100517 BK only blink icon if not charging...*/
        if (batpercent <= BATT_WARNING) { gtk_status_icon_set_blinking(tray_icon,TRUE); }
    }
    
    //update tooltip...
    memdisplaylong[0]=0;
    if (charging == 0) strcat(memdisplaylong,_("Battery discharging, capacity "));
    else strcat(memdisplaylong,_("Battery charging, capacity "));
    sprintf(strpercent,"%d",batpercent);
    strcat(memdisplaylong,strpercent);
    strcat(memdisplaylong,"%");
    gtk_status_icon_set_tooltip(tray_icon, memdisplaylong);
}

/**
 * As it says, it handles the click on the battery. 
 * 
 * This shows info about the battery, taken from either the state and info 
 * files using ACPI, or the uevent file for power_supply.
 * 
 * We use the legacy yaf-splash because it lets us keep the monospace 
 * formatting.
 * 
 * @param *status_icon The reference to the battery icon. Unused.
 * @param user_data The reference to the user data dir. Unused.
 */
void tray_icon_on_click(GtkStatusIcon *status_icon, gpointer user_data)
{
	// We need a BIG string. Rather be safe than sorry. 
	char command[512] = "yaf-splash -font monospace -display :0 -bg thistle -placement center -close box -text";
    if (pmtype == PMTYPE_ACPI) {
	    strcat(command, "\"`cat ");
	    strcat(command, batpathinfo);
	    strcat(command, " ");
	    strcat(command, batpathstate);
	    strcat(command,"`\" & ");
	    system(command);
    } else if (pmtype == PMTYPE_POWER_SUPPLY) {
		char command[512] = "yaf-splash -font monospace -display :0 -bg thistle -placement center -close box -text ";
//      "
		strcat(command, "\"`bash -c \"column -t -s $'\t' ");
//         <(
			strcat(command, "<(paste <(");
//				<(
				strcat(command, "cut -f3- -d_ ");
				strcat(command, batpathfallbackuevent);
				strcat(command, " | cut -f1 -d= | sed -e 's/_/ /g; s/.*/\\L&/; s/[[:graph:]]*/\\u&/g; s/$/:/')"
//				)
//				(
				strcat(command,"<(cut -f2 -d= ");
		strcat(command, batpathfallbackuevent);
		strcat(command, ") )\"`\" & ");
//      "
		system(command);
		
		/* 
		 * Basically, while uevent has the info and state files, 
		 * power_supply has the uevent file.
		 * 
		 * Which is ugly. 
		 * 
		 * Here is a sample uevent from an HP Pavillion laptop:
		 *
		 *		POWER_SUPPLY_NAME=BAT0
		 *		POWER_SUPPLY_STATUS=Discharging
		 *		POWER_SUPPLY_PRESENT=1
		 *		POWER_SUPPLY_TECHNOLOGY=Li-ion
		 *		POWER_SUPPLY_CYCLE_COUNT=0
		 *		POWER_SUPPLY_VOLTAGE_MIN_DESIGN=10800000
		 *		POWER_SUPPLY_VOLTAGE_NOW=10653000
		 *		POWER_SUPPLY_CURRENT_NOW=0
		 *		POWER_SUPPLY_CHARGE_FULL_DESIGN=4400000
		 *		POWER_SUPPLY_CHARGE_FULL=4400000
		 *		POWER_SUPPLY_CHARGE_NOW=1740000
		 *		POWER_SUPPLY_CAPACITY=39
		 *		POWER_SUPPLY_CAPACITY_LEVEL=Normal
		 *		POWER_SUPPLY_MODEL_NAME=Primary
		 *		POWER_SUPPLY_MANUFACTURER=Hewlett-Packard
		 *		POWER_SUPPLY_SERIAL_NUMBER=
		 *
		 * (Note that POWER_SUPPLY_CYCLE_COUNT normally has the actual cycle count, 
		 * but my HP laptop doesn't display that for some stupid reason)
		 * 
		 * We parse this to make it a bit more readable. 
		 * 
		 * So, let's break this beast command up. We have to use annoying line
		 * comments because of a star slash in the command. 
		 */
//		   column -t -s $'\t' formats the columns properly. The output of paste
//		   is put in here. 
// 
//		   The first pipe group is this: 
//
//		  "cut -f3- -d_ /sys/class/power_supply/BAT*/uevent | cut -f1 -d= | sed -e 's/_/ /g; s/.*/\L&/; s/[[:graph:]]*/\u&/g; s/$/:/'"
//
//			 "cut -f3- -d_ /sys/class/power_supply/BAT*/uevent" takes off the
//			 redundant POWER_SUPPLY_ prefix by cutting out anything before the
//			 first two underscores.
//
//			 "cut -f1 -d=" separates the first value by splitting the = sign.
//			 This will be like NAME, STATUS, PRESENT, etc.
//
//			 "sed -e 's/_/ /g; s/.*/\L&/; s/[[:graph:]]*/\u&/g; s/$/:/'" does
//			 three things:
//				 "s/_/ /g" replaces the underscores with spaces.
//				 "s/.*/\L&/" sets everything to Title Case.
//				 "s/$/:/" puts asthetic colons after each line. 
//		  The second pipe group is this:
//		  "cut -f2 -d= /sys/class/power_supply/BAT*/uevent"
//		  This simply selects the values by splitting the = sign and taking 
//		  the value after. 
		/* 
		 * All of this is put into paste, which puts the values side-by-side.
		 * Then column -t -s $'\t' formats it to align properly, $'\t' being the tab character. 
		 * The formatted output will look like this: 
		 *
		 *		Name:                BAT0
		 *		Status:              Discharging
		 *		Present:             1
		 *		Technology:          Li-ion
		 *		Cycle Count:         0
		 *		Voltage Min Design:  10800000
		 *		Voltage Now:         10653000
		 *		Current Now:         0
		 *		Charge Full Design:  4400000
		 *		Charge Full:         4400000
		 *		Charge Now:          1740000
		 *		Capacity:            39
		 *		Capacity Level:      Normal
		 *		Model Name:          Primary
		 *		Manufacturer:        Hewlett-Packard
		 *		Serial Number:
		 * 
		 * So, could I parse each value? Yes. 
		 * 
		 * Do I want to? No. 
		 * 
		 * Note that these are in microvolts and microamp hours instead of 
		 * millivolts and milliamp hours. The Linux kernel is weird like that,
		 * being all ACCURATE and that. 
		 */

	}
}

void tray_icon_on_menu(GtkStatusIcon *status_icon, guint button, 
						guint activate_time, gpointer user_data)
{
    printf("Popup menu\n");
}

/**
 * Loads the tray icons.
 */
static GtkStatusIcon *create_tray_icon() {

    tray_icon = gtk_status_icon_new();
    g_signal_connect(G_OBJECT(tray_icon), "activate", G_CALLBACK(tray_icon_on_click), NULL);
    g_signal_connect(G_OBJECT(tray_icon), "popup-menu", G_CALLBACK(tray_icon_on_menu), NULL);

    
    blank_pixbuf=gdk_pixbuf_new_from_file("/usr/share/pixmaps/puppy/blank.svg",&gerror);
    emptychg_pixbuf=gdk_pixbuf_new_from_file("/usr/share/pixmaps/puppy/emptychg.svg",&gerror);
    twentychg_pixbuf=gdk_pixbuf_new_from_file("/usr/share/pixmaps/puppy/twentychg.svg",&gerror);
    fortychg_pixbuf=gdk_pixbuf_new_from_file("/usr/share/pixmaps/puppy/fortychg.svg",&gerror);
    sixtychg_pixbuf=gdk_pixbuf_new_from_file("/usr/share/pixmaps/puppy/sixtychg.svg",&gerror);
    eightychg_pixbuf=gdk_pixbuf_new_from_file("/usr/share/pixmaps/puppy/eightychg.svg",&gerror);
    fullchg_pixbuf=gdk_pixbuf_new_from_file("/usr/share/pixmaps/puppy/fullchg.svg",&gerror);
    emptydis_pixbuf=gdk_pixbuf_new_from_file("/usr/share/pixmaps/puppy/emptydis.svg",&gerror);
    twentydis_pixbuf=gdk_pixbuf_new_from_file("/usr/share/pixmaps/puppy/twentydis.svg",&gerror);
    fortydis_pixbuf=gdk_pixbuf_new_from_file("/usr/share/pixmaps/puppy/fortydis.svg",&gerror);
    sixtydis_pixbuf=gdk_pixbuf_new_from_file("/usr/share/pixmaps/puppy/sixtydis.svg",&gerror);
    eightydis_pixbuf=gdk_pixbuf_new_from_file("/usr/share/pixmaps/puppy/eightydis.svg",&gerror);
    fulldis_pixbuf=gdk_pixbuf_new_from_file("/usr/share/pixmaps/puppy/fulldis.svg",&gerror);

    gtk_status_icon_set_from_pixbuf(tray_icon,blank_pixbuf);
    
    gtk_status_icon_set_tooltip(tray_icon, _("Battery charge"));
    gtk_status_icon_set_visible(tray_icon, TRUE);

    return tray_icon;
}
/**
 * Reads the ACPI values.
 * 
 * @return 0 for success, 1 for failure.
 */
int getACPIStatus() {
	fp = fopen(batpathinfo,"r");
	if (fp == NULL) {
		system("logger -t powerapplet 'batpathinfo is null.'");
		return 1;
	}
	while(!feof(fp) ) {
		fgets(inbuf1,sizeof inbuf1,fp);
		if (inbuf1 == NULL) {
			system("logger -t powerapplet 'Unable to read battpathinfo.'");
			return 1;
		}
		if (strncmp("design capacity",inbuf1,14) == 0) {
			sscanf(inbuf1,"%*s %*s %s",powerdesign);
			ndesigncapacity=atoi(powerdesign);
		}
		if (strncmp("last full capacity",inbuf1,16) == 0)   {
			sscanf(inbuf1,"%*s %*s %*s %s",powerdesign);
			nlastfullcapacity=atoi(powerdesign);
			break;
		}
	}
	fclose(fp);

	fp = fopen(batpathstate,"r");
	if (fp == NULL) {
		system("logger -t powerapplet 'batpathstate is null.'");
		return 1;
	}
	while(!feof(fp)) {
		fgets(inbuf1,sizeof inbuf1,fp);
		if (inbuf1 == NULL) {
			system("logger -t powerapplet 'Unable to read battpathstate.'");
			return 1;
		}
		if (strncmp("remaining capacity:",inbuf1,16) == 0)  {
			sscanf(inbuf1,"%*s %*s %s",powerremaining);
			npowerremaining=atoi(powerremaining);
		}
		if (strncmp("charging state:",inbuf1,14) == 0) {
			sscanf(inbuf1, "%*s %*s %s", inbuf2);
			if (strncmp("charged",inbuf2,7) == 0) charged=1;
			if (strncmp("discharging",inbuf2,11) == 0) charging=0;
		}
	}
	fclose(fp);
	
	//calc percentage charged... check nlastfullcapacity has a sane value...
	if (nlastfullcapacity > 400 && nlastfullcapacity < ndesigncapacity) batpercent=(npowerremaining*100)/nlastfullcapacity;
	else batpercent=(npowerremaining*100)/ndesigncapacity;
	
	if (charged) batpercent=100; /*101006*/
	
	return 0;
}
/**
 * Because getting the values from the fallback power_supply path is file-by-file
 * (unless we really want to parse uevent), we call this over and over to make it 
 * cleaner. 
 * 
 * @param *path the path to the config file. 
 * @param *out the output status
 * @return 0 for no errors, 1 for error.
 */
int getFallbackInt(char path[], int *out) {
	fp = fopen(path,"r");
	if (fp == NULL) {
		char output[64] = "logger -t powerapplet 'Error: Unable to read ";
		strcat(output, path);
		strcat(output, ". Aborting.'");
		system(output);
		fclose(fp);
		return 1;
	}
	while(!feof(fp)) {
		fgets(inbuf1,sizeof inbuf1,fp);
		if (inbuf1 == NULL) {
			system("logger -t powerapplet 'inbuf1 is null'");
			fclose(fp);
			return 1;
		}
		*out=atoi(inbuf1);
    }
    fclose(fp);
    return 0;
}
/*
 */
int getFallbackChargeStatus(char path[]) {
	fp = fopen(path,"r");
	if (fp == NULL)  {
		char output[64] = "logger -t powerapplet 'Error: Unable to read ";
		strcat(output, path);
		strcat(output, ". Aborting.'");
		system(output);
		return 1;
	}
	while(!feof(fp)) {
		fgets(inbuf1,sizeof inbuf1,fp);
		if (inbuf1 == NULL) {
			system("logger -t powerapplet 'inbuf1 is null'");
			fclose(fp);
			return 1;
		}
		if (strncmp(inbuf1,"Full",3) == 0) {
			charged=1;
			charging=1;
		}
		else if (strncmp(inbuf1,"Charging",7) == 0) {
			charged=0;
			charging=1;
		}
		else if (strncmp(inbuf1,"Discharging",7) == 0) {
			charged=0;
			charging=0;
		} 
		else {
			system("logger -t powerapplet 'Invalid value for charging status. Aborting.'");
			fclose(fp);
			return 1;
		}
    }
    fclose(fp);
    return 0;
}
/**
 * Some computers don't properly report battery stats to 
 * /proc/acpi/battery OR /proc/apm, but it is readable by the 
 * power supply class in the kernel. 
 * 
 * This causes many false negatives, as while /proc/acpi exists, 
 * /proc/acpi/battery is missing, which is the same behavior that
 * desktops plugged directly into a wall have.
 * 
 * This may be related to computers with wonky EFI boot methods, 
 * such as Apple's built-in EFI.
 * 
 * @param *mcntbats - reference to cntbats in main()
 * @return exit status. 0 is ok, 1 is error. 
 */
int getFallbackBatteryInfo(int *mcntbats, DIR *dp, struct dirent *ep) {
	int energy = 0;
	dp = opendir("/sys/class/power_supply");
	if (dp != NULL) {
		while (ep = readdir (dp)) {
			if (strcmp(ep->d_name,".") == 0) continue;
			if (strcmp(ep->d_name,"..") == 0) continue;
			if (strcmp(ep->d_name,"") == 0) continue;
			if (strncmp(ep->d_name,"AC",1) == 0) continue; /* power_supply has both batteries and AC adapters in the same dir */
			if (strncmp(ep->d_name,"ADP",2) == 0) continue;
			*mcntbats=*mcntbats+1;
			if (*mcntbats > 1) continue;
			strcpy(batname,ep->d_name);
			// Copy the right battery number to all paths.
			strcat(batpathfallback,batname);
			strcat(batpathfallbackchargenowtmp,batname);
			strcat(batpathfallbackenergynowtmp,batname);
			strcat(batpathfallbackchargenow,batname);
			strcat(batpathfallbackchargefull,batname);
			strcat(batpathfallbackchargefulldesign,batname);
			strcat(batpathfallbackstatus,batname);
			strcat(batpathfallbackuevent,batname);
			
			// Some use charge_now, others use energy_now.
			strcat(batpathfallbackchargenowtmp,"/charge_now");
			if ((fp = fopen(batpathfallbackchargenowtmp,"r")) == NULL) {
				strcat(batpathfallbackenergynowtmp,"/energy_now");
				fclose(fp);
				if ((fp = fopen(batpathfallbackenergynowtmp,"r")) != NULL) {
					energy = 1;
				}
				else {
					system("logger -t powerapplet 'Can't find battery files. Abort.'");
					return 1;
				}
				fclose(fp);
			}
			else fclose(fp);
			// Okay, go on with the show.
			// We set the paths to the correct file. 
			if (energy == 0) {
				strcat(batpathfallbackchargenow, "/charge_now"); 
				strcat(batpathfallbackchargefull, "/charge_full"); 
				strcat(batpathfallbackchargefulldesign, "/charge_full_design"); 
			} else {
				strcat(batpathfallbackchargenow, "/energy_now"); 
				strcat(batpathfallbackchargefull, "/energy_full"); 
				strcat(batpathfallbackchargefulldesign, "/energy_full_design"); 
			}
			strcat(batpathfallbackstatus, "/status"); 
			strcat(batpathfallbackuevent, "/uevent"); 
		}
		(void) closedir(dp);
		pmtype = PMTYPE_POWER_SUPPLY;
		// No batteries found.
		if (*mcntbats == 0) {
			system("logger -t powerapplet 'No batteries found in /sys/class/power_supply.'");
			return 1;
		}
	}
	/*
	 *  This should never happen, as even on desktops, /sys/class/power_supply
	 * will at least be empty.
	 */ 
	else {
		system("logger -t powerapplet '/sys/class/power_supply is null'");
		return 1;
	}
	return 0;
}
/**
 * Main. Detects the battery stat type and starts the services. 
 */
int main(int argc, char **argv) {
  DIR *dp;
  struct dirent *ep;
  int cntbats;
  
  setlocale( LC_ALL, "" );
  bindtextdomain( "powerapplet_tray", "/usr/share/locale" );
  textdomain( "powerapplet_tray" );
	
    //apm or acpi?...
    pmtype=PMTYPE_ACPI; /*110929 2.6.39.4 kernel does not have /proc/acpi/info*/
    if((fp = fopen("/proc/apm","r")) != NULL) { pmtype=PMTYPE_APM; fclose(fp); }
    /*110929 if((fp = fopen("/proc/acpi/info","r")) != NULL) { pmtype=2; fclose(fp); }
    if (pmtype == 0) { system("logger -t powerapplet 'Abort, no /proc/apm or acpi/info'"); return 1; }*/

    if (pmtype == PMTYPE_ACPI) {
        cntbats=0;
        dp = opendir ("/proc/acpi/battery");
        if (dp != NULL) {
            while (ep = readdir (dp)) {
                if (strcmp(ep->d_name,".") == 0) continue;
		        if (strcmp(ep->d_name,"..") == 0) continue;
		        if (strcmp(ep->d_name,"") == 0) continue;
		        cntbats=cntbats+1;
		        if (cntbats > 1) continue;
		        strcpy(batname,ep->d_name);
		        strcat(batpathinfo,batname);
		        strcat(batpathinfo,"/info"); /*ex: /proc/acpi/battery/BAT0/info*/
		        strcat(batpathstate,batname);
		        strcat(batpathstate,"/state");
            }
            (void) closedir(dp);
        }
        else {
            system("logger -t powerapplet 'Unable to open /proc/acpi/battery. Trying /sys/class/power_supply/.'");
            if (getFallbackBatteryInfo(&cntbats, dp, ep) != 0) {
				return 1;
			}
        }
        if (cntbats == 0) {
            system("logger -t powerapplet 'Unable to find any batteries.'");
            if (getFallbackBatteryInfo(&cntbats, dp, ep) != 0) {
				return 1;
			}
        }
        if((fp = fopen(batpathinfo,"r")) != NULL) fclose(fp);
        else {
            system("logger -t powerapplet 'Unable to find info file in /proc/acpi/battery'");
            if (getFallbackBatteryInfo(&cntbats, dp, ep) != 0) {
				return 1;
			}
        }
    }

    GtkStatusIcon *tray_icon;

    gtk_init(&argc, &argv);
    tray_icon = create_tray_icon();
    
    gtk_timeout_add(interval, Update, NULL);
    Update(NULL);

    gtk_main();

    return 0;
}
