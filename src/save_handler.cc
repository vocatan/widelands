/*
 * Copyright (C) 2002-2009 by the Widelands Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "save_handler.h"

#include <functional>
#include <thread>

#include "chat.h"
#include "game_io/game_saver.h"
#include "io/filesystem/filesystem.h"
#include "log.h"
#include "logic/game.h"
#include "profile/profile.h"
#include "wexception.h"
#include "wlapplication.h"
#include "wui/interactive_player.h"

using Widelands::Game_Saver;

void SaveHandler::save_worker
	(Widelands::Game& game, std::string filename, std::function<void(std::string)> onError,
	 std::function<void()> onSuccess)
{
	m_save_mutex.lock();
	log("save worker: starting...\n");
	bool const binary =
		!g_options.pull_section("global").get_bool("nozip", false);
	// Make sure that the base directory exists
	g_fs->EnsureDirectoryExists(get_base_dir());

	// Make a filesystem out of this
	std::unique_ptr<FileSystem> fs;
	if (!binary) {
		fs.reset(g_fs->CreateSubFileSystem(filename, FileSystem::DIR));
	} else {
		fs.reset(g_fs->CreateSubFileSystem(filename, FileSystem::ZIP));
	}

	Game_Saver gs(*fs, game);
	WLApplication* app = WLApplication::get();
	try {
		gs.save();
		m_save_mutex.unlock();
	} catch (const std::exception & e) {
		m_save_mutex.unlock();
		if (onError) {
			std::function<void()> error_funct = std::bind(onError, std::string(e.what()));
			app->post_runnable(error_funct);
		}
		log("save worker: Error caught, quitting...\n");
		return;
	}
	if (onSuccess) {
		app->post_runnable(onSuccess);
	}
	log("save worker: done...\n");
}

void SaveHandler::save
	(Widelands::Game& game, const std::string& filename,
	 std::function< void(std::string) > onError,
    std::function< void() > onSuccess)
{
	log("Launching save thread\n");
	std::thread save_thread(&SaveHandler::save_worker, this, std::ref(game), filename, onError, onSuccess);
	save_thread.detach();
	log("Save thread launched\n");
}

/**
* Check if autosave is not needed.
 */
void SaveHandler::think(Widelands::Game & game, int32_t realtime) {
	initialize(realtime);
	std::string filename = "wl_autosave";

	if (!m_allow_saving) {
		return;
	}
	const int32_t autosave_interval_in_seconds =
			g_options.pull_section("global").get_int
				("autosave", DEFAULT_AUTOSAVE_INTERVAL * 60);

	if (m_save_requested) {
		if (!m_save_filename.empty()) {
			filename = m_save_filename;
		}

		log("Autosave: save requested : %s\n", filename.c_str());
		m_save_requested = false;
		m_save_filename = "";
	} else {
		if (autosave_interval_in_seconds <= 0) {
			return; // no autosave requested
		}

		const int32_t elapsed = (realtime - m_last_saved_time) / 1000;
		if (elapsed < autosave_interval_in_seconds) {
			return;
		}

		log("Autosave: interval elapsed (%d s), saving\n", elapsed);
		m_last_saved_time = realtime;
	}

	// Prepare filename
	const std::string complete_filename = create_file_name(get_base_dir(), filename);
	std::string backup_filename;

	// always overwrite a file
	if (g_fs->FileExists(complete_filename)) {
		filename += "2";
		backup_filename = create_file_name (get_base_dir(), filename);
		if (g_fs->FileExists(backup_filename)) {
			g_fs->Unlink(backup_filename);
		}
		g_fs->Rename(complete_filename, backup_filename);
	}

	// Prepare the save job
	game.get_ipl()->get_chat_provider()->send_local
		(_("Saving game..."));

	std::function< void(std::string) > on_error_funct =
		[&game, complete_filename, backup_filename, realtime, autosave_interval_in_seconds, this]
			(std::string error) {
		log("Autosave: ERROR! - %s\n", error.c_str());
		game.get_ipl()->get_chat_provider()->send_local
			(_("Saving failed!"));

		// if backup file was created, move it back
		if (backup_filename.length() > 0) {
			if (g_fs->FileExists(complete_filename)) {
				g_fs->Unlink(complete_filename);
			}
			g_fs->Rename(backup_filename, complete_filename);
		}
		// Wait 30 seconds until next save try
		m_last_saved_time = realtime - autosave_interval_in_seconds * 1000 + 30000;
	};

	std::function<void()> on_success_funct =
		[backup_filename, &game, this]() {
		if (backup_filename.length() > 0 && g_fs->FileExists(backup_filename)) {
			g_fs->Unlink(backup_filename);
		}
		game.get_ipl()->get_chat_provider()->send_local
			(_("Game saved"));
	};

	// Launch the save worker
	save(game, complete_filename, on_error_funct, on_success_funct);
}

/**
* Initialize autosave timer
 */
void SaveHandler::initialize(int32_t currenttime) {
	if (m_initialized)
		return;

	m_last_saved_time = currenttime;
	m_initialized = true;
}

/*
 * Calculate the name of the save file
 */
std::string SaveHandler::create_file_name
	(std::string dir, std::string filename)
{
	// ok, first check if the extension matches (ignoring case)
	bool assign_extension = true;
	if (filename.size() >= strlen(WLGF_SUFFIX)) {
		char buffer[10]; // enough for the extension
		filename.copy
			(buffer, sizeof(WLGF_SUFFIX), filename.size() - strlen(WLGF_SUFFIX));
		if (!strncasecmp(buffer, WLGF_SUFFIX, strlen(WLGF_SUFFIX)))
			assign_extension = false;
	}
	if (assign_extension)
		filename += WLGF_SUFFIX;

	// Now append directory name
	std::string complete_filename = dir;
	complete_filename += "/";
	complete_filename += filename;

	return complete_filename;
}

/*
 * Save the game
 *
 * returns true if saved
 */
bool SaveHandler::save_game
	(Widelands::Game   &       game,
	 const std::string &       complete_filename,
	 std::string       * const error)
{
	m_save_mutex.lock();
	bool const binary =
		!g_options.pull_section("global").get_bool("nozip", false);
	// Make sure that the base directory exists
	g_fs->EnsureDirectoryExists(get_base_dir());

	// Make a filesystem out of this
	std::unique_ptr<FileSystem> fs;
	if (!binary) {
		fs.reset(g_fs->CreateSubFileSystem(complete_filename, FileSystem::DIR));
	} else {
		fs.reset(g_fs->CreateSubFileSystem(complete_filename, FileSystem::ZIP));
	}

	bool result = true;
	Game_Saver gs(*fs, game);
	try {
		gs.save();
		m_save_mutex.unlock();
	} catch (const std::exception & e) {
		m_save_mutex.unlock();
		if (error)
			*error = e.what();
		result = false;
	}

	if (result)
		m_last_saved_time = WLApplication::get()->get_time();

	return result;
}

