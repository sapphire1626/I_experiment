use byteorder::{BigEndian, ReadBytesExt};
use rodio::{OutputStream, Sink, Source};
use std::io::{Read};
use std::net::{TcpStream, UdpSocket};
use std::thread;
use std::time::Duration;

const RTP_HEADER_LEN: usize = 12;
const SAMPLE_RATE: u32 = 44100;
const CHANNELS: u16 = 2;

fn main() {
    let mut stream = TcpStream::connect("124.32.255.155:16260").expect("TCP接続失敗");
    let mut buf = [0u8; 2];
    stream.read_exact(&mut buf).expect("ポート番号受信失敗");
    let udp_port = u16::from_be_bytes(buf);
    println!("UDPポート番号: {}", udp_port);

    let socket = UdpSocket::bind("0.0.0.0:0").expect("UDPバインド失敗");
    socket.connect(("124.32.255.155", udp_port)).expect("UDP接続失敗");

    let (_stream, stream_handle) = OutputStream::try_default().expect("rodio出力失敗");
    let sink = Sink::try_new(&stream_handle).expect("rodio sink失敗");

    thread::spawn(move || {
        let mut rtp_buf = [0u8; 1500];
        loop {
            match socket.recv(&mut rtp_buf) {
                Ok(size) if size > RTP_HEADER_LEN => {
                    let payload_type = rtp_buf[1] & 0x7F;
                    if payload_type != 11 {
                        continue; // L16以外は無視
                    }
                    let payload = &rtp_buf[RTP_HEADER_LEN..size];
                    let mut samples = Vec::with_capacity(payload.len() / 2);
                    let mut rdr = std::io::Cursor::new(payload);
                    while let Ok(sample) = rdr.read_i16::<BigEndian>() {
                        samples.push(sample);
                    }
                    let source = rodio::buffer::SamplesBuffer::new(CHANNELS as u16, SAMPLE_RATE, samples);
                    sink.append(source);
                }
                Ok(_) => {}
                Err(e) => {
                    eprintln!("UDP受信エラー: {}", e);
                    thread::sleep(Duration::from_millis(100));
                }
            }
        }
    });

    loop {
        thread::sleep(Duration::from_secs(1));
    }
}
