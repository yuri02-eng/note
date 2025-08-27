// 测试 async/await
async function fetchData() {
    const res = await fetch('https://api.example.com');
    return res.json();
}