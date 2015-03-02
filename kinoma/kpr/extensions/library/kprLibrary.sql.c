/*
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
*/
static char *kprLibrarySQLInstructions =
	"DROP VIEW IF EXISTS \"audio_view\";\
	DROP VIEW IF EXISTS \"image_view\";\
	DROP VIEW IF EXISTS \"video_view\";\
	DROP TABLE IF EXISTS \"file\";\
	DROP TABLE IF EXISTS \"audio\";\
	DROP TABLE IF EXISTS \"image\";\
	DROP TABLE IF EXISTS \"video\";\
	DROP TABLE IF EXISTS \"album\";\
	DROP TABLE IF EXISTS \"artist\";\
	DROP TABLE IF EXISTS \"genre\";\
	DROP TABLE IF EXISTS \"extension\";\
	DROP TABLE IF EXISTS \"thumbnail\";\
	CREATE TABLE file (\
		id INTEGER PRIMARY KEY NOT NULL,\
		path TEXT NOT NULL,\
		date REAL NOT NULL,\
		size INTEGER NOT NULL,\
		title TEXT NOT NULL\
	);\
	CREATE TABLE creation (\
		id INTEGER PRIMARY KEY NOT NULL\
	);\
	CREATE TABLE deletion (\
		id INTEGER PRIMARY KEY NOT NULL\
	);\
	CREATE TABLE modification (\
		id INTEGER PRIMARY KEY NOT NULL\
	);\
	CREATE INDEX file_path_index ON file ( path );\
	CREATE TABLE audio (\
		id INTEGER PRIMARY KEY NOT NULL,\
		mime TEXT NULL,\
		duration REAL DEFAULT 0 NOT NULL,\
		time REAL DEFAULT 0 NOT NULL,\
		artist_id INTEGER NULL REFERENCES artist,\
		album_id INTEGER NULL REFERENCES album,\
		taken REAL NULL,\
		track INTEGER NULL,\
		genre_id INTEGER NULL REFERENCES genre\
	);\
	CREATE TABLE image (\
		id INTEGER PRIMARY KEY NOT NULL,\
		mime TEXT NULL,\
		taken REAL NULL,\
		rotation INTEGER NOT NULL,\
		width INTEGER NOT NULL,\
		height INTEGER NOT NULL,\
		area INTEGER NOT NULL\
	);\
	CREATE TABLE video (\
		id INTEGER PRIMARY KEY NOT NULL,\
		mime TEXT NULL,\
		duration REAL DEFAULT 0 NOT NULL,\
		time REAL DEFAULT 0 NOT NULL,\
		width INTEGER NOT NULL,\
		height INTEGER NOT NULL,\
		area INTEGER NOT NULL\
	);\
	CREATE TABLE artist (\
		artist_id INTEGER PRIMARY KEY AUTOINCREMENT,\
		artist_name TEXT NOT NULL UNIQUE\
	);\
	CREATE TABLE album (\
		album_id INTEGER PRIMARY KEY AUTOINCREMENT,\
		album_name TEXT NOT NULL,\
		artist_id INTEGER NULL REFERENCES artist,\
		UNIQUE(album_name, artist_id)\
	);\
	CREATE TABLE genre (\
		genre_id INTEGER PRIMARY KEY AUTOINCREMENT,\
		genre_name TEXT NOT NULL UNIQUE\
	);\
	CREATE TABLE thumbnail (\
		id INTEGER PRIMARY KEY NOT NULL,\
		thumbnail_jpeg BLOB NULL,\
		thumbnail_width INTEGER NULL,\
		thumbnail_height INTEGER NULL\
	);\
	CREATE VIEW audio_view AS\
		SELECT\
			id,\
			date,\
			mime,\
			size,\
			path,\
			title,\
			duration,\
			time,\
			artist_name,\
			album_name,\
			taken,\
			track,\
			genre_name\
		FROM file NATURAL JOIN audio NATURAL JOIN artist NATURAL JOIN album NATURAL JOIN genre;\
	CREATE VIEW image_view AS\
		SELECT\
			id,\
			date,\
			mime,\
			size,\
			path,\
			title,\
			taken,\
			rotation,\
			width,\
			height,\
			area\
		FROM file NATURAL JOIN image;\
	CREATE VIEW video_view AS\
		SELECT\
			id,\
			date,\
			mime,\
			size,\
			path,\
			title,\
			duration,\
			time,\
			width,\
			height,\
			area\
		FROM file NATURAL JOIN video;\
	CREATE TABLE extension (\
		extension_name TEXT NOT NULL UNIQUE\
	);\
	INSERT INTO extension ( extension_name ) VALUES ( \"aa\" );\
	INSERT INTO extension ( extension_name ) VALUES ( \"aif\" );\
	INSERT INTO extension ( extension_name ) VALUES ( \"aiff\" );\
	INSERT INTO extension ( extension_name ) VALUES ( \"asf\" );\
	INSERT INTO extension ( extension_name ) VALUES ( \"at3\" );\
	INSERT INTO extension ( extension_name ) VALUES ( \"avi\" );\
	INSERT INTO extension ( extension_name ) VALUES ( \"bmp\" );\
	INSERT INTO extension ( extension_name ) VALUES ( \"cda\" );\
	INSERT INTO extension ( extension_name ) VALUES ( \"flv\" );\
	INSERT INTO extension ( extension_name ) VALUES ( \"gif\" );\
	INSERT INTO extension ( extension_name ) VALUES ( \"jpeg\" );\
	INSERT INTO extension ( extension_name ) VALUES ( \"jpg\" );\
	INSERT INTO extension ( extension_name ) VALUES ( \"m4a\" );\
	INSERT INTO extension ( extension_name ) VALUES ( \"m4p\" );\
	INSERT INTO extension ( extension_name ) VALUES ( \"m4v\" );\
	INSERT INTO extension ( extension_name ) VALUES ( \"mov\" );\
	INSERT INTO extension ( extension_name ) VALUES ( \"mp3\" );\
	INSERT INTO extension ( extension_name ) VALUES ( \"mp4\" );\
	INSERT INTO extension ( extension_name ) VALUES ( \"mpeg\" );\
	INSERT INTO extension ( extension_name ) VALUES ( \"mpg\" );\
	INSERT INTO extension ( extension_name ) VALUES ( \"mqv\" );\
	INSERT INTO extension ( extension_name ) VALUES ( \"oma\" );\
	INSERT INTO extension ( extension_name ) VALUES ( \"oms\" );\
	INSERT INTO extension ( extension_name ) VALUES ( \"pct\" );\
	INSERT INTO extension ( extension_name ) VALUES ( \"pict\" );\
	INSERT INTO extension ( extension_name ) VALUES ( \"png\" );\
	INSERT INTO extension ( extension_name ) VALUES ( \"qt\" );\
	INSERT INTO extension ( extension_name ) VALUES ( \"svg\" );\
	INSERT INTO extension ( extension_name ) VALUES ( \"tif\" );\
	INSERT INTO extension ( extension_name ) VALUES ( \"tiff\" );\
	INSERT INTO extension ( extension_name ) VALUES ( \"wav\" );\
	INSERT INTO extension ( extension_name ) VALUES ( \"wma\" );\
	INSERT INTO extension ( extension_name ) VALUES ( \"wmv\" );";
