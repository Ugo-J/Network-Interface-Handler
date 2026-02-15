use std::time::Instant;

fn main() {
    let start = Instant::now();
    
    let mut count = 0u64;
    for _i in 0..1_000_000_000 {
        count += 1;
    }
    
    let duration = start.elapsed();
    println!("{}s", duration.as_secs_f64());
    
    // Prevent compiler from optimizing away the count
    std::hint::black_box(count);
}