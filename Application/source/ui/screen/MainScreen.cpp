#include "ListSong.hpp"
#include "MainScreen.hpp"
#include "Types.hpp"
#include "Utils.hpp"

namespace Screen {
    MainScreen::MainScreen(Main::Application * a) : Screen() {
        this->app = a;
        this->playingID = -1;
        this->playingDuration = 0;

        this->onButtonPress(Aether::Button::B, [this](){
            this->app->exit();
        });
    }

    void MainScreen::update(uint32_t dt) {
        // Update player element values
        PlaybackStatus ps = this->app->sysmodule()->status();
        if (ps == PlaybackStatus::Playing) {
            this->pause->setHidden(false);
            this->play->setHidden(true);
            if (this->play->focussed()) {
                this->setFocussed(this->pause);
            }

        } else if (ps == PlaybackStatus::Paused) {
            this->pause->setHidden(true);
            this->play->setHidden(false);
            if (this->pause->focussed()) {
                this->setFocussed(this->play);
            }

        } else if (ps == PlaybackStatus::Stopped) {
            this->pause->setHidden(true);
            this->play->setHidden(false);
            if (this->pause->focussed()) {
                this->setFocussed(this->play);
            }

        } else {
            // Throw error overlay or something
        }

        // Update playback position
        double per = this->app->sysmodule()->position();
        this->position->setString(Utils::secondsToHMS(this->playingDuration * (per / 100.0)));
        this->seekBar->setValue(per);

        // Update volume
        if (!this->volume->selected()) {
            this->volume->setValue(this->app->sysmodule()->volume());
        } else {
            this->app->sysmodule()->sendSetVolume(this->volume->value());
        }

        // Update song info
        SongID id = this->app->sysmodule()->currentSong();
        if (id != this->playingID) {
            this->playingID = id;
            SongInfo si = this->app->database()->getSongInfoForID(id);
            if (si.ID != -1) {
                this->trackName->setString(si.title);
                this->trackArtist->setString(si.artist);
                this->duration->setString(Utils::secondsToHMS(si.duration));
                this->playingDuration = si.duration;
            }
        }

        // Now update elements
        Screen::update(dt);
    }

    void MainScreen::resetState() {
        this->sideRP->setActivated(false);
        this->sideSongs->setActivated(false);
        this->sideArtists->setActivated(false);
        this->sideAlbums->setActivated(false);
        this->sideQueue->setActivated(false);
        this->app->threadQueue()->removeQueuedTasks();
        this->app->threadQueue()->waitUntilDone();

        // (Re)create list
        this->removeElement(this->list);
        this->list = new Aether::List(320, 180, 950, 410);
        this->list->setScrollBarColour(this->app->theme()->muted2());
        this->list->setShowScrollBar(true);
        this->addElement(this->list);
    }

    void MainScreen::setupQueue() {
        this->app->sysmodule()->sendGetQueue(0, this->app->sysmodule()->queueSize() - 1); // Start fetch operation as early as possible
        this->resetState();
        this->sideQueue->setActivated(true);
        this->heading->setString("Play Queue");

        // Create items for songs in queue
        unsigned int totalSecs = 0;
        while (!this->app->sysmodule()->queueChanged()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        std::vector<SongID> queue = this->app->sysmodule()->queue();
        size_t currentSong = this->app->sysmodule()->songIdx();
        for (size_t i = 0; i < queue.size(); i++) {
            SongInfo si = this->app->database()->getSongInfoForID(queue[i]);
            totalSecs += si.duration;
            CustomElm::ListSong * l = new CustomElm::ListSong(this->app->threadQueue());
            l->setTitleString(si.title);
            l->setArtistString(si.artist);
            l->setAlbumString(si.album);
            l->setLengthString(Utils::secondsToHMS(si.duration));
            l->setLineColour(this->app->theme()->muted2());
            l->setTextColour(currentSong == i ? this->app->theme()->accent() : this->app->theme()->FG());
            SongID id = queue[i];
            l->setCallback([this, id](){

            });
            this->list->addElement(l);

            if (i == 0) {
                l->setY(this->list->y() + 10);
            }
        }

        this->subLength->setString(Utils::secondsToHoursMins(totalSecs));
        this->subLength->setX(1205 - this->subLength->w());
        this->subTotal->setString(std::to_string(queue.size()) + (queue.size() == 1 ? " track" : " tracks" ));
        this->subTotal->setX(1205 - this->subTotal->w());
    }

    void MainScreen::setupSongs() {
        this->resetState();
        this->sideSongs->setActivated(true);
        this->heading->setString("Songs");

        // Create items for songs
        unsigned int totalSecs = 0;
        std::vector<SongInfo> si = this->app->database()->getAllSongInfo();
        for (size_t i = 0; i < si.size(); i++) {
            totalSecs += si[i].duration;
            CustomElm::ListSong * l = new CustomElm::ListSong(this->app->threadQueue());
            l->setTitleString(si[i].title);
            l->setArtistString(si[i].artist);
            l->setAlbumString(si[i].album);
            l->setLengthString(Utils::secondsToHMS(si[i].duration));
            l->setLineColour(this->app->theme()->muted2());
            l->setTextColour(this->app->theme()->FG());
            SongID id = si[i].ID;
            l->setCallback([this, id](){
                this->app->sysmodule()->sendAddToQueue(id);
            });
            this->list->addElement(l);

            if (i == 0) {
                l->setY(this->list->y() + 10);
            }
        }

        this->subLength->setString(Utils::secondsToHoursMins(totalSecs));
        this->subLength->setX(1205 - this->subLength->w());
        this->subTotal->setString(std::to_string(si.size()) + (si.size() == 1 ? " track" : " tracks" ));
        this->subTotal->setX(1205 - this->subTotal->w());
    }

    void MainScreen::onLoad() {
        // Add background images
        this->bg = new Aether::Image(0, 0, "romfs:/bg/main.png");
        this->addElement(this->bg);
        this->sidegrad = new Aether::Image(310, 15, "romfs:/misc/sidegradient.png");
        this->sidegrad->setColour(Aether::Colour{255, 255, 255, 200});
        this->addElement(this->sidegrad);

        // === SIDEBAR ===
        this->sideBg = new Aether::Rectangle(0, 0, 310, 590);
        this->sideBg->setColour(this->app->theme()->sideBG());
        this->addElement(this->sideBg);
        // User
        this->userBg = new Aether::Rectangle(25, 30, 200, 50, 10);
        this->userBg->setColour(this->app->theme()->muted2());
        this->addElement(this->userBg);
        this->userIcon = new Aether::Image(this->userBg->x(), this->userBg->y(), "romfs:/user.png");
        this->userIcon->setWH(50, 50);
        this->addElement(this->userIcon);
        this->userText = new Aether::Text(this->userIcon->x() + this->userIcon->w() + 8, 0, "Username", 26);
        this->userText->setY(this->userBg->y() + (this->userBg->h() - this->userText->h())/2);
        this->userText->setColour(this->app->theme()->FG());
        this->addElement(this->userText);

        // Settings
        this->settingsBg = new Aether::Rectangle(235, 30, 50, 50, 10);
        this->settingsBg->setColour(this->app->theme()->muted2());
        this->addElement(this->settingsBg);
        this->settingsIcon = new Aether::Image(this->settingsBg->x() + 10, this->settingsBg->y() + 10, "romfs:/icons/settings.png");
        this->addElement(this->settingsIcon);

        // Search bar
        this->search = new CustomElm::SearchBox(25, 130, 260);
        this->search->setBoxColour(this->app->theme()->muted2());
        this->search->setIconColour(this->app->theme()->FG());
        this->addElement(this->search);

        // Navigation list
        this->sideRP = new CustomElm::SideButton(10, 210, 290);
        this->sideRP->setIcon(new Aether::Image(0, 0, "romfs:/icons/clock.png"));
        this->sideRP->setText("Recently Played");
        this->sideRP->setActiveColour(this->app->theme()->accent());
        this->sideRP->setInactiveColour(this->app->theme()->FG());
        this->sideRP->setCallback([this](){

        });
        this->addElement(this->sideRP);
        this->sideSongs = new CustomElm::SideButton(10, this->sideRP->y() + 60, 290);
        this->sideSongs->setIcon(new Aether::Image(0, 0, "romfs:/icons/musicnote.png"));
        this->sideSongs->setText("Songs");
        this->sideSongs->setActiveColour(this->app->theme()->accent());
        this->sideSongs->setInactiveColour(this->app->theme()->FG());
        this->sideSongs->setCallback([this](){
            this->setupSongs();
        });
        this->addElement(this->sideSongs);
        this->sideArtists = new CustomElm::SideButton(10, this->sideSongs->y() + 60, 290);
        this->sideArtists->setIcon(new Aether::Image(0, 0, "romfs:/icons/user.png"));
        this->sideArtists->setText("Artists");
        this->sideArtists->setActiveColour(this->app->theme()->accent());
        this->sideArtists->setInactiveColour(this->app->theme()->FG());
        this->sideArtists->setCallback([this](){

        });
        this->addElement(this->sideArtists);
        this->sideAlbums = new CustomElm::SideButton(10, this->sideArtists->y() + 60, 290);
        this->sideAlbums->setIcon(new Aether::Image(0, 0, "romfs:/icons/disc.png"));
        this->sideAlbums->setText("Albums");
        this->sideAlbums->setActiveColour(this->app->theme()->accent());
        this->sideAlbums->setInactiveColour(this->app->theme()->FG());
        this->sideAlbums->setCallback([this](){

        });
        this->addElement(this->sideAlbums);
        this->sideSeparator = new Aether::Rectangle(30, this->sideAlbums->y() + 70, 250, 1);
        this->sideSeparator->setColour(this->app->theme()->muted2());
        this->addElement(this->sideSeparator);
        this->sideQueue = new CustomElm::SideButton(10, this->sideSeparator->y() + 10, 290);
        this->sideQueue->setIcon(new Aether::Image(0, 0, "romfs:/icons/queue.png"));
        this->sideQueue->setText("Play Queue");
        this->sideQueue->setActiveColour(this->app->theme()->accent());
        this->sideQueue->setInactiveColour(this->app->theme()->FG());
        this->sideQueue->setCallback([this](){
            this->setupQueue();
        });
        this->addElement(this->sideQueue);

        // === PLAYER ===
        this->playerBg = new Aether::Rectangle(0, 590, 1280, 130);
        this->playerBg->setColour(this->app->theme()->bottomBG());
        this->addElement(this->playerBg);
        // Album/song playing
        this->albumCover = new Aether::Image(10, 600, "romfs:/misc/noalbum.png");
        this->albumCover->setWH(110, 110);
        this->addElement(this->albumCover);
        this->trackName = new Aether::Text(140, 625, "Nothing playing!", 24);
        this->trackName->setColour(this->app->theme()->FG());
        this->addElement(this->trackName);
        this->trackArtist = new Aether::Text(140, 660, "Play a song", 18);
        this->trackArtist->setColour(this->app->theme()->muted());
        this->addElement(this->trackArtist);

        // Controls
        this->shuffle = new Aether::Image(480, 630, "romfs:/icons/shuffle.png");
        this->shuffle->setColour(this->app->theme()->muted());
        this->shuffle->setCallback([this]() {
            // Shuffle toggle here
        });
        this->addElement(this->shuffle);
        this->previous = new Aether::Image(550, 628, "romfs:/icons/previous.png");
        this->previous->setColour(this->app->theme()->FG());
        this->previous->setCallback([this]() {
            this->app->sysmodule()->sendPrevious();
        });
        this->addElement(this->previous);
        this->play = new Aether::Image(610, 610, "romfs:/icons/play.png");
        this->play->setColour(this->app->theme()->FG());
        this->play->setCallback([this]() {
            this->app->sysmodule()->sendResume();
        });
        this->play->setHidden(true);
        this->addElement(this->play);
        this->pause = new Aether::Image(610, 610, "romfs:/icons/pause.png");
        this->pause->setColour(this->app->theme()->FG());
        this->pause->setCallback([this]() {
            this->app->sysmodule()->sendPause();
        });
        this->addElement(this->pause);
        this->next = new Aether::Image(705, 628, "romfs:/icons/next.png");
        this->next->setColour(this->app->theme()->FG());
        this->next->setCallback([this]() {
            this->app->sysmodule()->sendNext();
        });
        this->addElement(this->next);
        this->repeat = new Aether::Image(770, 630, "romfs:/icons/repeat.png");
        this->repeat->setColour(this->app->theme()->muted());
        this->repeat->setCallback([this]() {
            // Repeat toggle here
        });
        this->addElement(this->repeat);

        // Seeking
        this->position = new Aether::Text(0, 0, "0:00", 18);
        this->position->setX(420 - this->position->w());
        this->position->setY(693 - this->position->h()/2);
        this->position->setColour(this->app->theme()->FG());
        this->addElement(this->position);
        this->seekBar = new CustomElm::Slider(440, 684, 400, 20, 8);
        this->seekBar->setBarBackgroundColour(this->app->theme()->muted2());
        this->seekBar->setBarForegroundColour(this->app->theme()->accent());
        this->seekBar->setKnobColour(this->app->theme()->FG());
        this->seekBar->setNudge(1);
        this->addElement(this->seekBar);
        this->duration = new Aether::Text(860, 0, "0:00", 18);
        this->duration->setY(693 - this->duration->h()/2);
        this->duration->setColour(this->app->theme()->FG());
        this->addElement(this->duration);

        // Volume + full screen
        this->volumeIcon = new Aether::Image(970, 638, "romfs:/icons/volume.png");
        this->volumeIcon->setColour(this->app->theme()->FG());
        this->volumeIcon->setCallback([this]() {
            // Toggle volume here
        });
        this->addElement(this->volumeIcon);
        this->volume = new CustomElm::Slider(1025, 644, 130, 20, 8);
        this->volume->setBarBackgroundColour(this->app->theme()->muted2());
        this->volume->setBarForegroundColour(this->app->theme()->accent());
        this->volume->setKnobColour(this->app->theme()->FG());
        this->volume->setNudge(5);
        this->volume->setValue(100.0);
        this->volume->setCallback([this]() {
            this->app->sysmodule()->sendSetVolume(this->volume->value());
        });
        this->addElement(this->volume);
        this->fullscreen = new Aether::Image(1195, 638, "romfs:/icons/fullscreen.png");
        this->fullscreen->setColour(this->app->theme()->muted());
        this->fullscreen->setCallback([this]() {
            // Change screen
        });
        this->addElement(this->fullscreen);

        // === MAIN ===
        // Headings
        this->heading = new Aether::Text(385, 40, "", 60);
        this->heading->setColour(this->app->theme()->FG());
        this->addElement(this->heading);
        this->subLength = new Aether::Text(1205, 80, "", 20);
        this->subLength->setColour(this->app->theme()->muted());
        this->addElement(this->subLength);
        this->subTotal = new Aether::Text(1205, this->subLength->y() - 25, "", 20);
        this->subTotal->setColour(this->app->theme()->muted());
        this->addElement(this->subTotal);

        // Add list headings
        this->titleH = new Aether::Text(385, 150, "Title", 20);
        this->titleH->setColour(this->app->theme()->muted());
        this->addElement(this->titleH);
        this->artistH = new Aether::Text(753, 150, "Artist", 20);
        this->artistH->setColour(this->app->theme()->muted());
        this->addElement(this->artistH);
        this->albumH = new Aether::Text(948, 150, "Album", 20);
        this->albumH->setColour(this->app->theme()->muted());
        this->addElement(this->albumH);
        this->lengthH = new Aether::Text(1205, 150, "Length", 20);
        this->lengthH->setX(this->lengthH->x() - this->lengthH->w());
        this->lengthH->setColour(this->app->theme()->muted());
        this->addElement(this->lengthH);

        // Set songs active
        this->list = nullptr;
        this->setFocussed(this->sideSongs);
        this->setupSongs();
    }

    void MainScreen::onUnload() {
        this->removeElement(this->bg);
        this->removeElement(this->sidegrad);
        this->removeElement(this->sideBg);
        this->removeElement(this->userBg);
        this->removeElement(this->userIcon);
        this->removeElement(this->userText);
        this->removeElement(this->settingsBg);
        this->removeElement(this->settingsIcon);
        this->removeElement(this->search);
        this->removeElement(this->sideRP);
        this->removeElement(this->sideSongs);
        this->removeElement(this->sideArtists);
        this->removeElement(this->sideAlbums);
        this->removeElement(this->sideSeparator);
        this->removeElement(this->sideQueue);
        this->removeElement(this->playerBg);
        this->removeElement(this->albumCover);
        this->removeElement(this->trackName);
        this->removeElement(this->trackArtist);
        this->removeElement(this->shuffle);
        this->removeElement(this->previous);
        this->removeElement(this->play);
        this->removeElement(this->pause);
        this->removeElement(this->next);
        this->removeElement(this->repeat);
        this->removeElement(this->position);
        this->removeElement(this->seekBar);
        this->removeElement(this->duration);
        this->removeElement(this->volumeIcon);
        this->removeElement(this->volume);
        this->removeElement(this->fullscreen);
        this->removeElement(this->heading);
        this->removeElement(this->subLength);
        this->removeElement(this->subTotal);
        this->removeElement(this->list);
    }
};