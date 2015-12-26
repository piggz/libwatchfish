/*
 *  libwatchfish - library with common functionality for SailfishOS smartwatch connector programs.
 *  Copyright (C) 2015 Javier S. Pedro <dev.git@javispedro.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "musiccontroller.h"
#include "musiccontroller_p.h"

namespace watchfish
{

MusicControllerPrivate::MusicControllerPrivate(MusicController *q)
	: q_ptr(q)
{
}

MusicControllerPrivate::~MusicControllerPrivate()
{
}

MusicController::MusicController(QObject *parent)
	: QObject(parent), d_ptr(new MusicControllerPrivate(this))
{
}

MusicController::~MusicController()
{
	delete d_ptr;
}

}
