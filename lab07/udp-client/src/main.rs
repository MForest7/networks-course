use core::time;
use std::net::UdpSocket;
use std::thread::sleep;
use std::time::Duration;

use chrono::{DateTime, Utc};

fn gen_msg(index: i32, time: DateTime<Utc>) -> String {
    format!("Ping {} {}", index, time.to_string())
}

fn main() {
    let to_addr = "127.0.0.1:34254";
    println!("PING {}", &to_addr);

    let socket = UdpSocket::bind("127.0.0.1:34354").unwrap();
    socket
        .set_read_timeout(Some(Duration::from_secs(1)))
        .unwrap();
    socket.set_ttl(64).unwrap();

    let packets_count = 10;
    let mut response_delays: Vec<f32> = vec![];
    let begin_time = Utc::now();

    for index in 0..packets_count {
        let send_time = Utc::now();
        let msg = gen_msg(index, send_time);
        socket.send_to(&msg.as_bytes(), to_addr).unwrap();

        let mut buf = [0; 512];
        match socket.recv_from(&mut buf) {
            Ok((length, response_from_addr)) => {
                let receive_time = Utc::now();
                let delay_in_ms = (receive_time - send_time).to_std().unwrap().as_secs_f32() * 1e3;
                response_delays.push(delay_in_ms);
                println!(
                    "{} bytes from {}: icmp_seq={} ttl={} time={}ms",
                    length,
                    response_from_addr,
                    index,
                    socket.ttl().unwrap(),
                    delay_in_ms
                );
            }
            Err(_err) => {
                println!("lost \"{}\"", &msg)
            }
        };

        sleep(time::Duration::from_secs(1));
    }

    let end_time = Utc::now();
    let loss_ratio =
        ((packets_count - response_delays.len() as i32) as f32) / (packets_count as f32);

    println!(
        "{} packets transmitted, {} received, {}% packet loss, time {}ms",
        packets_count,
        response_delays.len(),
        loss_ratio * 100.0,
        (end_time - begin_time).to_std().unwrap().as_secs_f32() * 1e3
    );

    println!(
        "rtt min/max/avg = {}ms, {}ms, {}ms",
        response_delays
            .clone()
            .into_iter()
            .reduce(f32::min)
            .unwrap(),
        response_delays
            .clone()
            .into_iter()
            .reduce(f32::max)
            .unwrap(),
        response_delays.iter().sum::<f32>() as f32 / response_delays.len() as f32
    );
}
