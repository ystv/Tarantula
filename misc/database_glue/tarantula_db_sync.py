import psycopg2
import sqlite3
import os

# Remote database server settings
db_server = "localhost"
db_port = 5432
db_user = "user"
db_pass = "pass"
db_name = "db"

# Local database files
filler_db = "/opt/Tarantula/datafiles/EventProcessor_Fill/filedata.db"
file_db = "/opt/Tarantula/datafiles/EventProcessor_Fill/lazydata.db"
core_db = "/opt/Tarantula/datafiles/coredata.db"

# Connect to remote database
conn = psycopg2.connect(host=db_server, port=db_port, user=db_user, password=db_pass, database=db_name)

# Read data for the schedule_fill_items table
cur = conn.cursor()

cur.execute("SELECT schedule_fill_items.video_id, (upper(substring(filename from 9 for (length(filename) - 12)))), "
            "device, item_type, extract('epoch' from duration) * 25, priority, '' "
            "FROM schedule_fill_items "
            "LEFT JOIN video_files ON schedule_fill_items.video_id = video_files.video_id "
            "LEFT JOIN video_file_types ON video_files.video_file_type_name = video_file_types.name "
            "LEFT JOIN videos ON schedule_fill_items.video_id = videos.id "
            "WHERE video_file_types.mode = 'schedule'")
result = cur.fetchall()

# Read data for the full lazy mode table
cur.execute("SELECT videos.id, (upper(substring(filename from 9 for (length(filename) - 12)))), 'Show', 'show', (extract('epoch' from duration)) * 25, "
	"CASE WHEN created_date > (current_date - interval '1 year') THEN '1' "
	     "WHEN created_date > (current_date - interval '2 years') THEN '3' "
	     "ELSE '5' "
	"END, "
	"COALESCE(NULLIF(videos.schedule_name, ''),NULLIF(video_boxes.display_name, ''), "
		"NULLIF(video_boxes.name,''),NULLIF(video_boxes.url_name,'')) "
	"FROM video_files "
	"LEFT JOIN videos ON videos.id = video_files.video_id "
	"LEFT JOIN video_boxes ON videos.video_box_id = video_boxes.id "
	"LEFT JOIN video_file_types ON video_files.video_file_type_name = video_file_types.name "
	"WHERE video_file_types.mode = 'schedule' AND duration > interval '0 seconds' AND is_enabled = 'true' AND schedule_fill_enable = 'true' "
	"AND video_id NOT IN "
		"(SELECT video_id FROM schedule_fill_items) "
        "GROUP BY videos.id, filename, duration, created_date, video_boxes.display_name, video_boxes.name, video_boxes.url_name, "
        "videos.display_name, videos.url_name, videos.schedule_name "
	"ORDER BY created_date DESC")

fullvideolist = cur.fetchall()

# Release the cursor
cur.close()

# Connect to the SQLite database
conn2 = sqlite3.connect(filler_db)

inscur = conn2.cursor()
inscur.execute("DELETE FROM items;")
inscur.executemany("INSERT INTO items (id, name, device, type, duration, weight, description) VALUES (?, ?, ?, ?, ?, ?, ?);", result)
inscur.close()
conn2.commit()
conn2.close()

conn2 = sqlite3.connect(file_db)
inscur = conn2.cursor()
inscur.execute("ATTACH '" + core_db + "' AS filelist")
inscur.execute("DELETE FROM items;")
inscur.executemany("INSERT INTO items (id, name, device, type, duration, weight, description) VALUES (?, ?, ?, ?, ?, ?, ?);", fullvideolist)
inscur.execute("DELETE FROM items WHERE id IN \
	(SELECT id FROM items \
		LEFT OUTER JOIN filelist.[main video server_files] f ON f.filename = items.name \
		WHERE f.filename IS NULL)")
inscur.execute("DETACH filelist")
inscur.close()
conn2.commit()
conn2.close()


# Shut down the remote connection
conn.close()
