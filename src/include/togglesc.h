/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2008 Banco do Brasil S.A.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 *	@file togglesc.h
 *	@brief Global declarations for toggles.c.
 */

#pragma once

#include <lib3270/toggle.h>

LIB3270_INTERNAL void initialize_toggles(H3270 *session);
LIB3270_INTERNAL void shutdown_toggles(H3270 *session);
LIB3270_INTERNAL const LIB3270_TOGGLE toggle_descriptor[LIB3270_TOGGLE_COUNT+1];
LIB3270_INTERNAL void toggle_rectselect(H3270 *hSession, const struct lib3270_toggle *t, LIB3270_TOGGLE_TYPE tt);

