use chrono::{DateTime, TimeDelta, Utc};
use rand::Rng;
use std::collections::HashMap;
use std::env;
use std::net::UdpSocket;
use std::str::from_utf8;

fn parse_message(msg: &str) -> (i32, DateTime<Utc>) {
    let tokens: Vec<&str> = msg.splitn(3, ' ').collect();
    let index: i32 = tokens[1].parse().unwrap();
    let timestamp: DateTime<Utc> = tokens[2].parse().unwrap();
    (index, timestamp)
}

fn main() -> std::io::Result<()> {
    let socket = UdpSocket::bind("127.0.0.1:34254")?;
    let args: Vec<String> = env::args().collect();
    let timeout = TimeDelta::seconds(args[1].parse::<i64>().unwrap());

    socket
        .set_read_timeout(Some(timeout.to_std().unwrap()))
        .unwrap();
    let mut rng = rand::thread_rng();
    let mut buf = [0; 512];

    let mut clients = HashMap::<String, (i32, DateTime<Utc>)>::new();

    loop {
        match socket.recv_from(&mut buf) {
            Ok((length, from_addr)) => {
                let buf = &mut buf[..length];
                let (index, timestamp) = parse_message(from_utf8(&buf).unwrap());
                let lose_packet = rng.gen_bool(0.2);
                match clients.get(&from_addr.to_string()) {
                    Some((last_index, _)) => {
                        if !lose_packet {
                            let delay_in_ms =
                                (Utc::now() - timestamp).to_std().unwrap().as_secs_f32() * 1e3;
                            if index > last_index + 1 {
                                println!(
                                    "lost {} packets from {}",
                                    index - (last_index + 1),
                                    &from_addr.to_string()
                                );
                            }
                            println!("received from {} in {}ms", &from_addr, delay_in_ms);
                            clients.insert(from_addr.to_string(), (index, timestamp));
                        }
                    }
                    _ => {
                        clients.insert(from_addr.to_string(), (index, timestamp));
                    }
                };

                if !lose_packet {
                    socket.send_to(buf.to_ascii_uppercase().as_slice(), &from_addr)?;
                }
            }
            Err(_) => {}
        };

        let mut stopped_clients = Vec::new();
        for (client, (_, last_timestamp)) in clients.clone().into_iter() {
            if (Utc::now() - last_timestamp) > timeout {
                println!("client {} is now stopped", &client);
                stopped_clients.push(client.clone());
            }
        }
        for client in stopped_clients.into_iter() {
            clients.remove(&client);
        }
    }
}
