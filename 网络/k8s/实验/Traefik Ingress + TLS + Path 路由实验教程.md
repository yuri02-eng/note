# 🧪 **Traefik Ingress + TLS + Path 路由实验教程**

本实验通过 Traefik Ingress Controller 来实现：

* 支持 **HTTP → HTTPS 自动跳转**
* 使用自签名证书完成 **TLS 加密**
* 使用 Ingress **路径转发** 访问三个后端服务

最终可以实现在浏览器访问：

| URL                                            | 实际后端 |
| ---------------------------------------------- | ---- |
| [https://demo.local/s1](https://demo.local/s1) | svc1 |
| [https://demo.local/s2](https://demo.local/s2) | svc2 |
| [https://demo.local/](https://demo.local/)     | svc3 |

---

# 📘 **目录**

1. 实验背景知识
2. 创建 TLS 证书
3. 创建 Traefik 配置文件
4. 部署 Traefik Ingress Controller（NodePort 方式）
5. 部署 3 个 nginx 后端服务
6. 创建 Ingress 实现路径路由
7. 浏览器访问测试
8. 原理总结

---

# 1️⃣ **实验背景知识**

本实验的核心组件是 Traefik，它可以作为：

* Kubernetes Ingress Controller
* 提供 HTTP/HTTPS EntryPoint
* 提供 Dashboard
* 实现基于路径的路由规则

## 为什么要配置 TLS？

因为实际业务访问多数需要 HTTPS，本实验采用 **自签证书** 来模拟真实 HTTPS 的访问过程，帮助理解 Ingress TLS 的配置原理。

## 为什么要使用 Path 路由？

因为一个域名下面，可能需要访问多个内部服务：

* `/s1` → 第一个服务
* `/s2` → 第二个服务
* `/` → 默认服务

通过 Traefik，我们可以非常轻松地实现访问控制。

---

# 2️⃣ **创建 TLS 证书（含解释）**

Traefik HTTPS 需要证书，因此先创建自签名证书。

> ❗注意：浏览器会认为自签证书不受信任，因此访问时会出现“连接不安全”警告，这是正常的。

创建证书：

```bash
openssl req -newkey rsa:2048 -nodes \
  -keyout tls.key \
  -x509 -days 365 \
  -out tls.crt \
  -subj "/CN=demo.local"
```

解释：

* `-nodes` → 不加密私钥
* `-x509` → 输出为证书文件
* `/CN=demo.local` → 域名（浏览器校验 CN）

创建 secret，让 Kubernetes 能管理证书：

```bash
kubectl create secret generic traefik-cert \
  --from-file=tls.crt \
  --from-file=tls.key \
  -n kube-system
```

---

# 3️⃣ **Traefik 配置文件（traefik.toml）**

Traefik 需要一个配置文件来定义：

* http、https 入口
* http 强制跳转 https
* 使用我们的证书
* dashboard 开启

创建 `traefik.toml`：

```toml
defaultEntryPoints = ["http", "https"]

[entryPoints]
  [entryPoints.http]
  address = ":80"
    [entryPoints.http.redirect]
      entryPoint = "https"

  [entryPoints.https]
  address = ":443"
    [entryPoints.https.tls]
      [[entryPoints.https.tls.certificates]]
      CertFile = "/ssl/tls.crt"
      KeyFile = "/ssl/tls.key"

[api]
  dashboard = true
```

### 内容解释：

* Traefik 监听 80 和 443 端口
* 访问 http 自动跳到 https
* https 使用我们挂载到 Pod 内的证书
* Dashboard 开启（用来查看路由情况）

将配置文件存成 ConfigMap：

```bash
kubectl create configmap traefik-conf \
  --from-file=traefik.toml -n kube-system
```

---

# 4️⃣ **部署 Traefik Ingress Controller**

Traefik 是本实验的核心，因此必须部署。
采用 NodePort 的方式，方便在物理机浏览器访问。

创建 `traefik.yaml`（已优化）：

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: traefik-ingress-controller
  namespace: kube-system
spec:
  replicas: 1
  selector:
    matchLabels:
      app: traefik-ingress
  template:
    metadata:
      labels:
        app: traefik-ingress
    spec:
      volumes:
      - name: ssl
        secret:
          secretName: traefik-cert
      - name: config
        configMap:
          name: traefik-conf
      containers:
      - name: traefik
        image: traefik:v2.10
        args:
          - --configfile=/config/traefik.toml
          - --providers.kubernetesingress
        ports:
        - name: web
          containerPort: 80
        - name: websecure
          containerPort: 443
        - name: dashboard
          containerPort: 8080
        volumeMounts:
        - mountPath: "/ssl"
          name: ssl
        - mountPath: "/config"
          name: config
---
apiVersion: v1
kind: Service
metadata:
  name: traefik
  namespace: kube-system
spec:
  type: NodePort
  selector:
    app: traefik-ingress
  ports:
  - name: web
    port: 80
    nodePort: 30080
  - name: websecure
    port: 443
    nodePort: 32004
  - name: dashboard
    port: 8080
    nodePort: 30081
```

应用：

```bash
kubectl apply -f traefik.yaml
```

Traefik Dashboard（HTTP）：

```
http://<NodeIP>:30081/dashboard/
```

---

# 5️⃣ **部署后端服务（重要讲解版）**

使用 nginx 容器，但每个服务展示不同内容：

* svc1 → 显示 “svc1”
* svc2 → 显示 “svc2”
* svc3 → 显示 “svc3”

创建 `backend.yaml`：

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: svc1
spec:
  replicas: 1
  selector:
    matchLabels:
      app: svc1
  template:
    metadata:
      labels:
        app: svc1
    spec:
      containers:
      - name: svc1
        image: nginx:latest
        command: ["/bin/sh","-c"]
        args:
          - echo "<h1>svc1</h1>" > /usr/share/nginx/html/s1 && nginx -g 'daemon off;'
        ports:
        - containerPort: 80
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: svc2
spec:
  replicas: 1
  selector:
    matchLabels:
      app: svc2
  template:
    metadata:
      labels:
        app: svc2
    spec:
      containers:
      - name: svc2
        image: nginx:latest
        command: ["/bin/sh","-c"]
        args:
          - echo "<h1>svc2</h1>" > /usr/share/nginx/html/s2 && nginx -g 'daemon off;'
        ports:
        - containerPort: 80
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: svc3
spec:
  replicas: 1
  selector:
    matchLabels:
      app: svc3
  template:
    metadata:
      labels:
        app: svc3
    spec:
      containers:
      - name: svc3
        image: nginx:latest
        command: ["/bin/sh","-c"]
        args:
          - echo "<h1>svc3</h1>" > /usr/share/nginx/html/index.html && nginx -g 'daemon off;'
        ports:
        - containerPort: 80
---
apiVersion: v1
kind: Service
metadata:
  name: svc1
spec:
  selector:
    app: svc1
  ports:
    - port: 80
---
apiVersion: v1
kind: Service
metadata:
  name: svc2
spec:
  selector:
    app: svc2
  ports:
    - port: 80
---
apiVersion: v1
kind: Service
metadata:
  name: svc3
spec:
  selector:
    app: svc3
  ports:
    - port: 80
```

应用：

```bash
kubectl apply -f backend.yaml
```

---

# 6️⃣ **创建 Ingress 路由（Path 转发）**

创建 `ingress.yaml`：

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: demo-ingress
  annotations:
    kubernetes.io/ingress.class: "traefik"
spec:
  tls:
    - hosts:
        - demo.local
      secretName: traefik-cert
  rules:
  - host: demo.local
    http:
      paths:
      - path: /s1
        pathType: Prefix
        backend:
          service:
            name: svc1
            port:
              number: 80
      - path: /s2
        pathType: Prefix
        backend:
          service:
            name: svc2
            port:
              number: 80
      - path: /
        pathType: Prefix
        backend:
          service:
            name: svc3
            port:
              number: 80
```

应用：

```bash
kubectl apply -f ingress.yaml
```

---

# 7️⃣ **访问验证（最关键的部分）**

首先添加 hosts：

```
sudo nano /etc/hosts
192.168.241.138 demo.local
```

---

## 🔹访问根路径（svc3）

```
https://demo.local:32004/
```

输出：

```
svc3
```

---

## 🔹访问 svc1

```
https://demo.local:32004/s1
```

---

## 🔹访问 svc2

```
https://demo.local:32004/s2
```

---

# 8️⃣ **实验原理总结（非常重要）**

### ✔ Traefik 监听 NodePort：32004 → Pod 443

浏览器访问：

```
https://demo.local:32004
```

Traefik NodePort → Deployment → https entrypoint → Ingress → 后端 Service → Pod

### ✔ Path 路由规则

* `/s1` → svc1
* `/s2` → svc2
* `/` → svc3（默认路由）

### ✔ 证书为什么会提示“不安全”？

因为自签证书没有权威机构签发。生产环境要使用 CA 证书（Let’s Encrypt 或企业证书）。

---

# 🎉 **到这里你已经完成了一套完整的 Traefik + TLS + Path 实验教程！**
