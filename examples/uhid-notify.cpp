/**
 * This exposes a new core, NotifySQLiteCore, as a stand-alone U2F implementationof for Linux Desktops.
 *
 * The new extends SQLiteCore to use libnotify to display user presence confirmation popups.
 *
 * Unfortunately, the multithreading is necessarialy messy, since the popup and the U2F messages are asynchnous with each other:
 * - The popup must be shown whevener user presence is checked
 * - The popup must be closed after a 5s timout without u2F messages checking
 *   for user presence, or when presence is confirmed.
 * - After presence is confirmed, U2F will have only 5s to check for it.
 */

#include <u2f/core-sqlite.h>
#include <u2f/hid.h>
#include <hiddev/uhid.h>
#include <stdio.h>
#include <thread>
#include <chrono>

#include <libnotify/notify.h>
#include <gtk/gtk.h>

#define LOG(fmt, ...) fprintf(stderr, "u2f-notify: " fmt "\n", ##__VA_ARGS__)

using namespace std::chrono_literals;
using std::chrono::steady_clock;

class NotifySQLiteCore : public u2f::SQLiteCore {
public:
	steady_clock::time_point userPresentUntil;
	steady_clock::time_point notififyUntil;

	std::mutex notifyMutex;
	std::thread *notifyTimeoutThread;
	std::condition_variable notifyTimeoutCondition;
	std::condition_variable notifyFinishedCondition;

	NotifyNotification *notification;

	NotifySQLiteCore(const char* filename)
	:	u2f::SQLiteCore(filename),
		notification(nullptr),
		notifyTimeoutThread(nullptr),
		userPresentUntil(steady_clock::now()),
		notififyUntil(steady_clock::now())
	{ }

	~NotifySQLiteCore() {
		std::unique_lock<std::mutex> lck(notifyMutex);
		closeNotification(lck);
	}

	static void notifyThread(NotifySQLiteCore* core) {
		std::unique_lock<std::mutex> lck(core->notifyMutex);
		while (steady_clock::now() < core->notififyUntil) {
			core->notifyTimeoutCondition.wait_until(lck, core->notififyUntil);
		}
		LOG("Closing notification");
		notify_notification_close(core->notification, nullptr);
		g_object_unref(G_OBJECT(core->notification));
		core->notification = nullptr;
		core->notifyFinishedCondition.notify_all();
	}

	void showNotification(std::unique_lock<std::mutex> &lck) {
		notififyUntil = steady_clock::now() + 5000ms;

		if (!notification) {
			LOG("Showing notification");

			if (notifyTimeoutThread) { // Delete old threads
				notifyTimeoutThread->join();
				delete notifyTimeoutThread;
				notifyTimeoutThread = nullptr;
			}

			notification = notify_notification_new ("U2F Authentication", "Confirm authentication", "dialog-information");

			g_signal_connect (notification, "closed", G_CALLBACK(notificationClosed), this);

			notify_notification_add_action(notification, "presence", "Confirm", userIsPresent, this, nullptr);
			notify_notification_set_timeout (notification, NOTIFY_EXPIRES_NEVER);
			notify_notification_set_urgency (notification, NOTIFY_URGENCY_CRITICAL);

			GError *error = nullptr;
			notify_notification_show(notification, &error);

			if (error) {
				LOG("Failed to show notification: %s\n", error->message);
				g_error_free(error);
				g_object_unref(G_OBJECT(notification));
				notification = nullptr;
			} else {
				notifyTimeoutThread = new std::thread(notifyThread, this);
			}
		}
	}

	void closeNotification(std::unique_lock<std::mutex> &lck, bool userPresent=false) {
		if (userPresent)
			userPresentUntil = steady_clock::now() + 5000ms;

		if (notifyTimeoutThread) {
			if (notification) {
				notififyUntil = steady_clock::now();
				notifyTimeoutCondition.notify_all();
				notifyFinishedCondition.wait(lck);
			}
			notifyTimeoutThread->join();
			delete notifyTimeoutThread;
			notifyTimeoutThread = nullptr;
		}
	}

	static void notificationClosed(NotifyNotification *notification, gpointer user_data) {
		NotifySQLiteCore* core = (NotifySQLiteCore*) user_data;
		std::unique_lock<std::mutex> lck(core->notifyMutex);
		core->closeNotification(lck);
	}

	static void userIsPresent(NotifyNotification *notification, char *action, gpointer user_data) {
		NotifySQLiteCore* core = (NotifySQLiteCore*) user_data;
		std::unique_lock<std::mutex> lck(core->notifyMutex);
		core->closeNotification(lck, true);
	}

	virtual bool isUserPresent() {
		std::unique_lock<std::mutex> lck(notifyMutex);
		if (steady_clock::now() < userPresentUntil) {
			closeNotification(lck);
			userPresentUntil = steady_clock::now();
			return true;
		} else {
			showNotification(lck);
			return false;
		}
	}
};

int main() {
	notify_init ("u2fdev");

	NotifySQLiteCore core("handles.db");
	u2f::Hid hid(core);
	hiddev::UHid uhid(hid);
	std::future<bool> uhid_runner = uhid.runAsync();

	gtk_main();
	return 0;
}
