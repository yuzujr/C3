# C3 Setup Guide

Complete installation and configuration guide for C3 remote control system.

## Prerequisites

- Node.js
- PostgreSQL (for native installation)
- Docker and Docker Compose (for Docker installation)

## Setup Methods

### Method 1: Native Installation

#### 1. Install Dependencies

```bash
cd server
npm install
```

#### 2. Database Configuration

**Install PostgreSQL:**
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install postgresql postgresql-contrib

# Start and enable PostgreSQL
sudo systemctl start postgresql
sudo systemctl enable postgresql
```

**Configure Authentication:**

PostgreSQL defaults to `ident` authentication, but applications need `md5` authentication. Modify the authentication configuration:

```bash
# Find and configure PostgreSQL authentication
HBA_FILE=$(sudo -u postgres psql -t -c "SHOW hba_file;" | xargs)
sudo cp "$HBA_FILE" "$HBA_FILE.backup"

# Modify authentication methods to md5
sudo sed -i 's/local   all             all                                     peer/local   all             all                                     md5/' "$HBA_FILE"
sudo sed -i 's/host    all             all             127.0.0.1\/32            ident/host    all             all             127.0.0.1\/32            md5/' "$HBA_FILE"
sudo sed -i 's/host    all             all             ::1\/128                 ident/host    all             all             ::1\/128                 md5/' "$HBA_FILE"

# Restart PostgreSQL to apply changes
sudo systemctl restart postgresql
```

**Create Database and User:**
```bash
sudo -u postgres psql
```

In PostgreSQL command line:
```sql
CREATE USER c3user WITH PASSWORD 'your-database-password';
CREATE DATABASE c3_db OWNER c3user;
GRANT ALL PRIVILEGES ON DATABASE c3_db TO c3user;
\q
```

**Important**: Replace `'your-database-password'` with a secure password of your choice. You will need to use the same password in your `.env` file's `DB_PASSWORD` setting.

#### 3. Configure Environment Variables

```bash
# Use the template for native installation
cp .env.example .env
```

Edit `.env` file with your database password and authentication settings:
- Set `DB_PASSWORD` to match the password you used when creating the database user
- Set `AUTH_USERNAME` and `AUTH_PASSWORD` for web interface login
- Set `SESSION_SECRET` to a long random string (at least 32 characters)
- Set `BASE_PATH` to the URL path prefix if deploying under a subdirectory (e.g., `/c3` for `http://domain.com/c3/`), leave empty for root deployment

**Note:** Other configuration parameters not detailed here are also important for specific use cases. Please refer to the complete [configuration reference](#configuration-reference) section below for all available options.

Check configuration security:
```bash
node scripts/check-security.js
```

#### 4. Initialize Database

```bash
# Test database connection
node scripts/test-database.js

# Initialize database tables and default user
node database/init.js
```

**Note:** The database schema is automatically created using Sequelize ORM models. The `init.js` script will create all necessary tables, indexes, and relationships, as well as create a default admin user.

#### 5. Start Server

```bash
node app.js
```

#### 6. Access Web Interface

Open browser and navigate to: **http://localhost:3000**

### Method 2: Docker Deployment

#### 1. Prepare Configuration

```bash
cd server
# Use the template for Docker deployment
cp .env.docker .env
```

#### 2. Start Services

```bash
docker-compose up -d
```

#### 3. Verify Deployment

```bash
# Check service status
docker-compose ps

# View logs
docker-compose logs -f c3-server
```

#### 4. Access Web Interface

Open browser and navigate to: **http://localhost:3000**

## Client Setup

### 1. Get Client

**Option 1: Download from [Releases](https://github.com/yuzujr/C3/releases) (Recommended)**

**Option 2: Compile from Source**

See [BUILD.md](./BUILD.md) for detailed compilation instructions.

### 2. Configure Client

Move `config/config.json` next to the executable:

```json
{
  "api": {
    "hostname": "127.0.0.1",
    "port": 3000,
    "base_path": "",
    "use_ssl": false,
    "skip_ssl_verification": false,
    "interval_seconds": 60,
    "max_retries": 3,
    "retry_delay_ms": 1000,
    "add_to_startup": false,
    "client_id": ""
  }
}
```

### 3. Run Client

```bash
./build/src/C3
```


## Configuration Reference

### Environment Variables

| Variable | Default | Description |
| `BASE_PATH` | `""` | Base URL path prefix |
| `NODE_ENV` | `development` | Environment mode |
| `PORT` | `3000` | Server port |
| `HOST` | `0.0.0.0` | Server host |
| `AUTH_ENABLED` | `true` | Enable authentication |
| `AUTH_USERNAME` | `admin` | Login username |
| `AUTH_PASSWORD` | - | Login password |
| `SESSION_SECRET` | - | Session encryption key |
| `SESSION_EXPIRE_HOURS` | `24` | Session expiration |
| `DB_HOST` | `localhost` | Database host |
| `DB_PORT` | `5432` | Database port |
| `DB_NAME` | `c3_db` | Database name |
| `DB_USER` | `c3user` | Database username |
| `DB_PASSWORD` | - | Database password |
| `UPLOAD_DIR` | `uploads` | Upload directory |
| `MAX_FILE_SIZE` | `10485760` | Max file size (bytes) |
| `LOG_LEVEL` | `info` | Logging level |
| `DB_LOGGING` | `false` | Database query logging |

### Client Configuration

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `hostname` | string | `127.0.0.1` | Server hostname or IP |
| `port` | number | `3000` | Server port |
| `base_path` | string | `""` | Base URL path prefix (e.g., "/c3" for reverse proxy) |
| `use_ssl` | boolean | `false` | Enable HTTPS/WSS |
| `skip_ssl_verification` | boolean | `false` | Skip SSL verification ⚠️ |
| `interval_seconds` | number | `60` | Screenshot interval |
| `max_retries` | number | `3` | Upload retry attempts |
| `retry_delay_ms` | number | `1000` | Retry delay |
| `add_to_startup` | boolean | `false` | Add to system startup |
| `client_id` | string | `""` | Unique client ID |

## Management Tools

```bash
# Security check
node scripts/check-security.js

# Database test
node scripts/test-database.js

# Database reset（Warning)
./scripts/reset-database.sh
```