import psycopg2
import sqlite3
import os

# Remote database server settings
db_server = "127.0.0.1"
db_port = 5432
db_user = "user"
db_pass = "pass"
db_name = "dbname"

# Local database files
filler_db = "/home/sam.nicholson/Code/Tarantula/datafiles/EventProcessor_Fill/filedata.db"


# Connect to remote database
conn = psycopg2.connect(host=db_server, port=db_port, user=db_user, password=db_pass, database=db_name)

# Read data for the schedule_fill_items table
cur = conn.cursor()

cur.execute("SELECT schedule_fill_items.video_id, substring(filename from 9), "
            "device, item_type, extract('epoch' from duration) * 25, priority "
            "FROM schedule_fill_items "
            "LEFT JOIN video_files ON schedule_fill_items.video_id = video_files.video_id "
            "LEFT JOIN video_file_types ON video_files.video_file_type_name = video_file_types.name "
            "LEFT JOIN videos ON schedule_fill_items.video_id = videos.id "
            "WHERE video_file_types.mode = 'schedule'")
result = cur.fetchall()

# Release the cursor
cur.close()

# Assemble the insert query
if (len(result) > 0):
    insertq = "INSERT INTO 'items' SELECT " + str(result[0][0]) + " AS 'video_id', '" + str(os.path.splitext(result[0][1])[0]).upper() + \
        "' AS 'name', '" + result[0][2] + "' AS 'device', '" + result[0][3] + "' AS 'type', " + str(int(result[0][4])) + \
        " AS 'duration', " + str(result[0][5]) + " AS 'weight'"
    
    for row in result[1:]:
        insertq += " UNION SELECT " + str(row[0]) + ", '" + str(os.path.splitext(row[1])[0]).upper() + "', '" + row[2] + "', '" + \
        row[3] + "', " + str(int(row[4])) + ", " + str(row[5])
        
    # Connect to the SQLite database
    conn2 = sqlite3.connect(filler_db)
    
    inscur = conn2.cursor()
    
    inscur.execute("DELETE FROM items")
    inscur.execute(insertq)
    
    conn2.commit()
    conn2.close()


# Shut down the remote connection
conn.close()
