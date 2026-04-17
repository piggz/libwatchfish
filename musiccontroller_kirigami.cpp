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

// Volume control for desktop / Kirigami using the native PulseAudio C API
// (libpulse). Public API exposes int 0-100, mapped to PA_VOLUME_NORM
// internally.

#include <pulse/pulseaudio.h>

#include "musiccontroller.h"
#include "musiccontroller_p.h"

namespace watchfish {

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

MusicControllerPrivate::~MusicControllerPrivate() { disconnectPulse(); }

void MusicControllerPrivate::disconnectPulse() {
  if (_paContext) {
    pa_context_disconnect(_paContext);
    pa_context_unref(_paContext);
    _paContext = nullptr;
  }
  if (_paMainloop) {
    pa_mainloop_free(_paMainloop);
    _paMainloop = nullptr;
  }
  _cachedVolume = -1;
}

bool MusicControllerPrivate::connectPulse() {
  // Re-use an already connected context.
  if (_paContext && pa_context_get_state(_paContext) == PA_CONTEXT_READY) {
    return true;
  }

  // Clean up any stale state.
  disconnectPulse();

  _paMainloop = pa_mainloop_new();
  if (!_paMainloop) {
    qWarning() << "Failed to create PulseAudio mainloop";
    return false;
  }

  pa_mainloop_api *api = pa_mainloop_get_api(_paMainloop);
  _paContext = pa_context_new(api, "libwatchfish");
  if (!_paContext) {
    qWarning() << "Failed to create PulseAudio context";
    pa_mainloop_free(_paMainloop);
    _paMainloop = nullptr;
    return false;
  }

  if (pa_context_connect(_paContext, nullptr, PA_CONTEXT_NOFLAGS, nullptr) <
      0) {
    qWarning() << "pa_context_connect() failed:"
               << pa_strerror(pa_context_errno(_paContext));
    disconnectPulse();
    return false;
  }

  // Wait until the context reaches a terminal state.
  int retval = 0;
  for (;;) {
    if (pa_mainloop_iterate(_paMainloop, 1, &retval) < 0) {
      qWarning() << "PulseAudio mainloop error during connect";
      disconnectPulse();
      return false;
    }
    pa_context_state_t state = pa_context_get_state(_paContext);
    if (state == PA_CONTEXT_READY) {
      break;
    }
    if (!PA_CONTEXT_IS_GOOD(state)) {
      qWarning() << "PulseAudio context failed to connect:"
                 << pa_strerror(pa_context_errno(_paContext));
      disconnectPulse();
      return false;
    }
  }

  qDebug() << "Connected to PulseAudio server";
  return true;
}

/**
 * Run the mainloop until @p op completes (or is cancelled/errored).
 * Returns true if the operation finished successfully.
 */
bool MusicControllerPrivate::iterateMainloop(pa_operation *op) {
  if (!op) {
    return false;
  }

  int retval = 0;
  while (pa_operation_get_state(op) == PA_OPERATION_RUNNING) {
    if (pa_mainloop_iterate(_paMainloop, 1, &retval) < 0) {
      qWarning() << "PulseAudio mainloop error";
      pa_operation_unref(op);
      return false;
    }
  }

  bool ok = (pa_operation_get_state(op) == PA_OPERATION_DONE);
  pa_operation_unref(op);
  return ok;
}

// ---------------------------------------------------------------------------
// Callbacks (free functions used as C callbacks)
// ---------------------------------------------------------------------------

struct SinkInfoResult {
  int volume = -1; // 0-100
  bool finished = false;
};

static void sinkInfoCallback(pa_context * /*ctx*/, const pa_sink_info *info,
                             int eol, void *userdata) {
  auto *result = static_cast<SinkInfoResult *>(userdata);
  if (eol > 0) {
    result->finished = true;
    return;
  }
  if (!info) {
    result->finished = true;
    return;
  }
  // Convert the average channel volume to 0-100.
  pa_volume_t avg = pa_cvolume_avg(&info->volume);
  // PA_VOLUME_NORM is 100 % (65536). Clamp to 0-100.
  int vol = static_cast<int>(
      qBound(0u, static_cast<uint>(avg * 100u / PA_VOLUME_NORM), 100u));
  result->volume = vol;
  result->finished = true;
}

struct SetVolumeResult {
  bool finished = false;
};

static void setVolumeCallback(pa_context * /*ctx*/, int /*success*/,
                              void *userdata) {
  auto *result = static_cast<SetVolumeResult *>(userdata);
  result->finished = true;
}

// ---------------------------------------------------------------------------
// MusicController public API
// ---------------------------------------------------------------------------

int MusicController::volume() const {
  if (!d_ptr->connectPulse()) {
    return -1;
  }

  SinkInfoResult result;
  pa_operation *op = pa_context_get_sink_info_by_name(
      d_ptr->_paContext, "@DEFAULT_SINK@", sinkInfoCallback, &result);

  if (!d_ptr->iterateMainloop(op)) {
    return -1;
  }

  d_ptr->_cachedVolume = result.volume;
  return result.volume;
}

void MusicController::setVolume(const uint newVolume) {
  qDebug() << "Setting volume:" << newVolume;

  if (!d_ptr->connectPulse()) {
    qWarning() << "PulseAudio not available";
    return;
  }

  // Map 0-100 to 0-PA_VOLUME_NORM.
  pa_volume_t paVol = static_cast<pa_volume_t>(qBound(0u, newVolume, 100u) *
                                               PA_VOLUME_NORM / 100u);

  // We need the channel count of the default sink first.
  // Retrieve it via a sink info query.
  SinkInfoResult infoResult;
  pa_operation *infoOp = pa_context_get_sink_info_by_name(
      d_ptr->_paContext, "@DEFAULT_SINK@", sinkInfoCallback, &infoResult);

  if (!d_ptr->iterateMainloop(infoOp) || infoResult.volume < 0) {
    qWarning() << "Failed to query default sink info";
    return;
  }

  // Build the volume structure using the channel count reported above.
  // We re-query just for the channel count; reuse the callback result.
  // Because sinkInfoCallback only stores the average volume we need a
  // dedicated struct for the channel count. Use a lambda-shaped approach
  // via a small local struct and a second query.
  struct SinkChannelsResult {
    uint8_t channels = 0;
    bool finished = false;
  } chResult;

  pa_operation *chOp = pa_context_get_sink_info_by_name(
      d_ptr->_paContext, "@DEFAULT_SINK@",
      [](pa_context * /*c*/, const pa_sink_info *info, int eol, void *ud) {
        auto *r = static_cast<SinkChannelsResult *>(ud);
        if (eol > 0 || !info) {
          r->finished = true;
          return;
        }
        r->channels = info->volume.channels;
        r->finished = true;
      },
      &chResult);

  if (!d_ptr->iterateMainloop(chOp) || chResult.channels == 0) {
    qWarning() << "Failed to query default sink channel count";
    return;
  }

  pa_cvolume vol;
  pa_cvolume_set(&vol, chResult.channels, paVol);

  SetVolumeResult setResult;
  pa_operation *setOp = pa_context_set_sink_volume_by_name(
      d_ptr->_paContext, "@DEFAULT_SINK@", &vol, setVolumeCallback, &setResult);

  if (!d_ptr->iterateMainloop(setOp)) {
    qWarning() << "Failed to set PulseAudio volume";
    return;
  }

  if (d_ptr->_cachedVolume != static_cast<int>(newVolume)) {
    emit volumeChanged();
  }

  d_ptr->_cachedVolume = static_cast<int>(newVolume);
}

void MusicController::volumeUp() {
  int cur = volume();
  if (cur < 0) {
    return;
  }
  if (cur >= 100) {
    qDebug() << "Cannot increase volume beyond maximum (100)";
    return;
  }
  setVolume(static_cast<uint>(qMin(cur + 5, 100)));
}

void MusicController::volumeDown() {
  int cur = volume();
  if (cur <= 0) {
    qDebug() << "Cannot decrease volume beyond minimum (0)";
    return;
  }
  setVolume(static_cast<uint>(qMax(cur - 5, 0)));
}

} // namespace watchfish
