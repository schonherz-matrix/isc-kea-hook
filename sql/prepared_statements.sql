-- check_switch
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
update mueb m set port_id   = null, switch_id = null from port p where m.port_id = p.port_id and m.switch_id = p.switch_id and p.room_id = (select r.room_id from room r join port p using (room_id) where p.port_id = :port_id and p.switch_id = :switch_id);

-- insert_or_update_mueb
insert into mueb (mac_address) values (:mac_address) on conflict (mac_address) do update set port_id = :port_id, switch_id = :switch_id where mueb.mac_address = :mac_address;

-- set_ip_conflict
update mueb m set ip_conflict = true where m.mac_address = :mac_address;