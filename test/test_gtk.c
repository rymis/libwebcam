#include <gtk/gtk.h>

static GtkWidget *main_window = NULL;
static GtkWidget *image = NULL;

static void create_window(void)
{
	GtkWidget *hbox;
	// GtkBox    *scales;

	main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	hbox = gtk_hbox_new(0, 0);
	gtk_container_add(GTK_CONTAINER(main_window), hbox);

	image = gtk_image_new();

	/* Signals: */
	g_signal_connect(main_window, "destroy", gtk_main_quit, NULL);

	gtk_widget_show_all(main_window);
}

int main(int argc, char *argv[])
{
	gtk_init(&argc, &argv);

	create_window();
	gtk_main();

	return 0;
}

