use byteorder::{BigEndian, ReadBytesExt};
use rodio::{OutputStream, Sink};
use rtp_rs::RtpPacketBuilder;
use rtp_rs::{RtpReader, Seq};
use std::env;
use std::io::Read;
use std::io::prelude::*;
use std::net::{TcpStream, UdpSocket};
use std::thread;
use std::time::Duration;

const RTP_HEADER_LEN: usize = 12;
const SAMPLE_RATE: u32 = 44100;
const DATA_SIZE: usize = 1024;

fn main() {
    let args: Vec<String> = env::args().collect();
    let server_ip = &args[1];
    let server_addr = format!("{}:16260", server_ip);
    let mut stream = TcpStream::connect(&server_addr).expect("TCP接続失敗");

    // holdフラグの送信
    let hold: u8 = 1;
    stream.write(&[hold]).expect("holdフラグ送信失敗");

    // 音声データの送受ポートの取得
    let mut buf = [0u8; 2];
    stream.read_exact(&mut buf).expect("ポート番号受信失敗");
    let udp_port = u16::from_be_bytes(buf);
    println!("UDPポート番号: {}", udp_port);

    let socket = UdpSocket::bind("0.0.0.0:0").expect("UDPバインド失敗");
    socket
        .connect((server_ip.as_str(), udp_port))
        .expect("UDP接続失敗");

    let (_stream, stream_handle) = OutputStream::try_default().expect("rodio出力失敗");
    let sink = Sink::try_new(&stream_handle).expect("rodio sink失敗");

    // サーバに認識してもらうためにダミーRTPパケットを1回だけ送信
    let dummy_payload = vec![0u8; DATA_SIZE];
    let rtp_dummy_packet = RtpPacketBuilder::new()
        .payload_type(11)
        .sequence(Seq::from(0))
        .timestamp(0)
        .ssrc(0)
        .payload(&dummy_payload)
        .build()
        .expect("RTPパケットビルド失敗");
    socket
        .send(&rtp_dummy_packet)
        .expect("RTPダミーパケット送信失敗");

    thread::spawn(move || {
        let mut rtp_buf = [0u8; DATA_SIZE + RTP_HEADER_LEN];
        loop {
            match socket.recv(&mut rtp_buf) {
                Ok(size) if size > RTP_HEADER_LEN => {
                    let rtp_packet = RtpReader::new(&rtp_buf).expect("RTPパケット読み込み失敗");
                    if rtp_packet.payload_type() != 11 {
                        eprintln!(
                            "無効なRTPパケット: payload_type={}",
                            rtp_packet.payload_type()
                        );
                        continue; // L16以外は無視
                    }
                    let payload = rtp_packet.payload();
                    let mut samples = Vec::with_capacity(payload.len() / 2);
                    let mut rdr = std::io::Cursor::new(payload);
                    while let Ok(sample) = rdr.read_i16::<BigEndian>() {
                        samples.push(sample);
                    }
                    let source = rodio::buffer::SamplesBuffer::new(1u16, SAMPLE_RATE, samples);
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
