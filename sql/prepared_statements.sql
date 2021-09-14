-- check switch
select count(*) from switch where switch_id = :switch_id;

-- ip_conflict
select ip_conflict::int from mueb where mac_address = :mac;

-- mueb_in_room
select room_id from port p join room r using(room_id) where p.port_id = :port_id and p.switch_id = :switch_id;

-- mueb_count_in_room
select count(*) from port p join mueb m using(port_id, switch_id) where m.mac_address != :mac_address and p.room_id = :room_id;

-- ip_override
select ip_override from mueb where mac_address = :mac and ip_override is not null;

-- ip_address
select ip_address from port p join room r using(room_id) where p.port_id = :port_id and p.switch_id = :switch_id;

-- clear_port
update mueb set port_id = null, switch_id = null where port_id = :port_id and switch_id = :switch_id;

-- insert_mueb
insert into mueb (mac_address) values (:mac_address) on conflict (mac_address) do update set port_id = :port_id, switch_id = :switch_id where mueb.mac_address = :mac_address;

-- set ip conflict
update mueb m set ip_conflict = true where m.mac_address = :mac_address;