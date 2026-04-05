mysql user and password:tester Test@123456
mysql> describe users;
+--------------+--------------+------+-----+-------------------+-----------------------------------------------+
| Field        | Type         | Null | Key | Default           | Extra                                         |
+--------------+--------------+------+-----+-------------------+-----------------------------------------------+
| id           | int          | NO   | PRI | NULL              |                                               |
| username     | varchar(50)  | NO   |     | NULL              |                                               |
| loginname    | varchar(50)  | NO   | UNI | NULL              |                                               |
| passwordhash | varchar(255) | NO   |     | NULL              |                                               |
| created_at   | timestamp    | YES  |     | CURRENT_TIMESTAMP | DEFAULT_GENERATED                             |
| updated_at   | timestamp    | YES  |     | CURRENT_TIMESTAMP | DEFAULT_GENERATED on update CURRENT_TIMESTAMP |
+--------------+--------------+------+-----+-------------------+-----------------------------------------------+


kafka pwd and start shell is
/opt/app/kafka_2.13-4.1.0/kafka_2.13-4.1.0
bin  config  kafka_service.sh  libs  LICENSE  licenses  logs  NOTICE  site-docs