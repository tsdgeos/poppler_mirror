/* poppler-movie.cc: glib interface to Movie
 *
 * Copyright (C) 2010 Carlos Garcia Campos <carlosgc@gnome.org>
 * Copyright (C) 2008 Hugo Mercier <hmercier31[@]gmail.com>
 * Copyright (C) 2017 Francesco Poli <invernomuto@paranoici.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "poppler-movie.h"
#include "poppler-private.h"

/**
 * SECTION: poppler-movie
 * @short_description: Movies
 * @title: PopplerMovie
 */

typedef struct _PopplerMovieClass PopplerMovieClass;

struct _PopplerMovie
{
    GObject parent_instance;

    gchar *filename;
    gboolean need_poster;
    gboolean show_controls;
    PopplerMoviePlayMode mode;
    gboolean synchronous_play;
    gdouble volume;
    gdouble rate;
    guint64 start;
    guint64 duration;
    gushort rotation_angle;
    gint width;
    gint height;
};

struct _PopplerMovieClass
{
    GObjectClass parent_class;
};

G_DEFINE_TYPE(PopplerMovie, poppler_movie, G_TYPE_OBJECT)

static void poppler_movie_finalize(GObject *object)
{
    PopplerMovie *movie = POPPLER_MOVIE(object);

    if (movie->filename) {
        g_free(movie->filename);
        movie->filename = nullptr;
    }

    G_OBJECT_CLASS(poppler_movie_parent_class)->finalize(object);
}

static void poppler_movie_class_init(PopplerMovieClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->finalize = poppler_movie_finalize;
}

static void poppler_movie_init(PopplerMovie *movie) { }

PopplerMovie *_poppler_movie_new(const Movie *poppler_movie)
{
    PopplerMovie *movie;

    g_assert(poppler_movie != nullptr);

    movie = POPPLER_MOVIE(g_object_new(POPPLER_TYPE_MOVIE, nullptr));

    movie->filename = g_strdup(poppler_movie->getFileName()->c_str());
    if (poppler_movie->getShowPoster()) {
        Object tmp = poppler_movie->getPoster();
        movie->need_poster = (!tmp.isRef() && !tmp.isStream());
    }

    movie->show_controls = poppler_movie->getActivationParameters()->showControls;

    switch (poppler_movie->getActivationParameters()->repeatMode) {
    case MovieActivationParameters::repeatModeOnce:
        movie->mode = POPPLER_MOVIE_PLAY_MODE_ONCE;
        break;
    case MovieActivationParameters::repeatModeOpen:
        movie->mode = POPPLER_MOVIE_PLAY_MODE_OPEN;
        break;
    case MovieActivationParameters::repeatModeRepeat:
        movie->mode = POPPLER_MOVIE_PLAY_MODE_REPEAT;
        break;
    case MovieActivationParameters::repeatModePalindrome:
        movie->mode = POPPLER_MOVIE_PLAY_MODE_PALINDROME;
        break;
    }

    movie->synchronous_play = poppler_movie->getActivationParameters()->synchronousPlay;

    // map 0 - 100 to 0.0 - 1.0
    movie->volume = poppler_movie->getActivationParameters()->volume / 100.0;

    movie->rate = poppler_movie->getActivationParameters()->rate;

    if (poppler_movie->getActivationParameters()->start.units_per_second > 0 && poppler_movie->getActivationParameters()->start.units <= G_MAXUINT64 / 1000000000) {
        movie->start = 1000000000L * poppler_movie->getActivationParameters()->start.units / poppler_movie->getActivationParameters()->start.units_per_second;
    } else {
        movie->start = 0L;
    }

    if (poppler_movie->getActivationParameters()->duration.units_per_second > 0 && poppler_movie->getActivationParameters()->duration.units <= G_MAXUINT64 / 1000000000) {
        movie->duration = 1000000000L * poppler_movie->getActivationParameters()->duration.units / poppler_movie->getActivationParameters()->duration.units_per_second;
    } else {
        movie->duration = 0L;
    }

    movie->rotation_angle = poppler_movie->getRotationAngle();

    poppler_movie->getAspect(&movie->width, &movie->height);

    return movie;
}

/**
 * poppler_movie_get_filename:
 * @poppler_movie: a #PopplerMovie
 *
 * Returns the local filename identifying a self-describing movie file
 *
 * Return value: a local filename, return value is owned by #PopplerMovie and
 *               should not be freed
 *
 * Since: 0.14
 */
const gchar *poppler_movie_get_filename(PopplerMovie *poppler_movie)
{
    g_return_val_if_fail(POPPLER_IS_MOVIE(poppler_movie), NULL);

    return poppler_movie->filename;
}

/**
 * poppler_movie_need_poster:
 * @poppler_movie: a #PopplerMovie
 *
 * Returns whether a poster image representing the Movie
 * shall be displayed. The poster image must be retrieved
 * from the movie file.
 *
 * Return value: %TRUE if move needs a poster image, %FALSE otherwise
 *
 * Since: 0.14
 */
gboolean poppler_movie_need_poster(PopplerMovie *poppler_movie)
{
    g_return_val_if_fail(POPPLER_IS_MOVIE(poppler_movie), FALSE);

    return poppler_movie->need_poster;
}

/**
 * poppler_movie_show_controls:
 * @poppler_movie: a #PopplerMovie
 *
 * Returns whether to display a movie controller bar while playing the movie
 *
 * Return value: %TRUE if controller bar should be displayed, %FALSE otherwise
 *
 * Since: 0.14
 */
gboolean poppler_movie_show_controls(PopplerMovie *poppler_movie)
{
    g_return_val_if_fail(POPPLER_IS_MOVIE(poppler_movie), FALSE);

    return poppler_movie->show_controls;
}

/**
 * poppler_movie_get_play_mode:
 * @poppler_movie: a #PopplerMovie
 *
 * Returns the play mode of @poppler_movie.
 *
 * Return value: a #PopplerMoviePlayMode.
 *
 * Since: 0.54
 */
PopplerMoviePlayMode poppler_movie_get_play_mode(PopplerMovie *poppler_movie)
{
    g_return_val_if_fail(POPPLER_IS_MOVIE(poppler_movie), POPPLER_MOVIE_PLAY_MODE_ONCE);

    return poppler_movie->mode;
}

/**
 * poppler_movie_is_synchronous:
 * @poppler_movie: a #PopplerMovie
 *
 * Returns whether the user must wait for the movie to be finished before
 * the PDF viewer accepts any interactive action
 *
 * Return value: %TRUE if yes, %FALSE otherwise
 *
 * Since: 0.80
 */
gboolean poppler_movie_is_synchronous(PopplerMovie *poppler_movie)
{
    g_return_val_if_fail(POPPLER_IS_MOVIE(poppler_movie), FALSE);

    return poppler_movie->synchronous_play;
}

/**
 * poppler_movie_get_volume:
 * @poppler_movie: a #PopplerMovie
 *
 * Returns the playback audio volume
 *
 * Return value: volume setting for the movie (0.0 - 1.0)
 *
 * Since: 0.80
 */
gdouble poppler_movie_get_volume(PopplerMovie *poppler_movie)
{
    g_return_val_if_fail(POPPLER_IS_MOVIE(poppler_movie), 0);

    return poppler_movie->volume;
}

/**
 * poppler_movie_get_rate:
 * @poppler_movie: a #PopplerMovie
 *
 * Returns the relative speed of the movie
 *
 * Return value: the relative speed of the movie (1 means no change)
 *
 * Since: 0.80
 */
gdouble poppler_movie_get_rate(PopplerMovie *poppler_movie)
{
    g_return_val_if_fail(POPPLER_IS_MOVIE(poppler_movie), 0);

    return poppler_movie->rate;
}

/**
 * poppler_movie_get_rotation_angle:
 * @poppler_movie: a #PopplerMovie
 *
 * Returns the rotation angle
 *
 * Return value: the number of degrees the movie should be rotated (positive,
 * multiples of 90: 0, 90, 180, 270)
 *
 * Since: 0.80
 */
gushort poppler_movie_get_rotation_angle(PopplerMovie *poppler_movie)
{
    g_return_val_if_fail(POPPLER_IS_MOVIE(poppler_movie), 0);

    return poppler_movie->rotation_angle;
}

/**
 * poppler_movie_get_start:
 * @poppler_movie: a #PopplerMovie
 *
 * Returns the start position of the movie playback
 *
 * Return value: the start position of the movie playback (in ns)
 *
 * Since: 0.80
 */
guint64 poppler_movie_get_start(PopplerMovie *poppler_movie)
{
    g_return_val_if_fail(POPPLER_IS_MOVIE(poppler_movie), 0L);

    return poppler_movie->start;
}

/**
 * poppler_movie_get_duration:
 * @poppler_movie: a #PopplerMovie
 *
 * Returns the duration of the movie playback
 *
 * Return value: the duration of the movie playback (in ns)
 *
 * Since: 0.80
 */
guint64 poppler_movie_get_duration(PopplerMovie *poppler_movie)
{
    g_return_val_if_fail(POPPLER_IS_MOVIE(poppler_movie), 0L);

    return poppler_movie->duration;
}

/**
 * poppler_movie_get_aspect:
 * @poppler_movie: a #PopplerMovie
 * @width: width of the movie's bounding box
 * @height: height of the movie's bounding box
 *
 * Returns the dimensions of the movie's bounding box (in pixels).
 * The respective PDF movie dictionary entry is optional; if missing,
 * -1x-1 will be returned.
 *
 * Since: 0.89
 */
void poppler_movie_get_aspect(PopplerMovie *poppler_movie, gint *width, gint *height)
{
    g_return_if_fail(POPPLER_IS_MOVIE(poppler_movie));

    *width = poppler_movie->width;
    *height = poppler_movie->height;
}
