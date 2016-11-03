CREATE TABLE messages(id varchar(0x100), dir integer(1), message varchar(0x100), date integer(4), id_match varchar(0x100), PRIMARY KEY(id), FOREIGN KEY(id_match) references matches(mid) ON DELETE CASCADE);
CREATE TABLE images_processed(url varchar(0x100), width integer(2), height integer(2), id_image varchar(0x100), FOREIGN KEY(id_image) references images(id) ON DELETE CASCADE);
CREATE TABLE persons(pid varchar(0x100), name varchar(0x100), birth integer(4), PRIMARY KEY(pid));
CREATE TABLE matches(mid varchar(0x100), date integer(4), id_person varchar(0x100), PRIMARY KEY(mid), FOREIGN KEY(id_person) references persons(pid) ON DELETE CASCADE);
CREATE TABLE recs(pid varchar(0x100), date integer(4), PRIMARY KEY(pid), FOREIGN KEY(pid) references persons(pid) ON DELETE CASCADE);
CREATE TABLE images(id varchar(0x100), url varchar(0x100), main integer(1), id_person varchar(0x100), PRIMARY KEY(id), FOREIGN KEY(id_person) references persons(pid) ON DELETE CASCADE);
