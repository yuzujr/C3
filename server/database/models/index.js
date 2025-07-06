// database/models/index.js
// 简化的数据库模型

const { Sequelize, DataTypes } = require('sequelize');
const config = require('../../config');

const sequelize = new Sequelize(
    config.DB_NAME,
    config.DB_USER,
    config.DB_PASSWORD,
    {
        host: config.DB_HOST,
        port: config.DB_PORT,
        dialect: 'postgres',
        logging: config.DB_LOGGING,
        define: {
            timestamps: true,
            createdAt: 'created_at',
            updatedAt: 'updated_at'
        },
        pool: {
            max: 5,
            min: 0,
            acquire: 30000,
            idle: 10000
        }
    }
);

// 定义模型
const User = sequelize.define('User', {
    username: {
        type: DataTypes.STRING(50),
        allowNull: false,
        unique: true
    },
    password_hash: {
        type: DataTypes.STRING(255),
        allowNull: false
    },
    role: {
        type: DataTypes.STRING(20),
        defaultValue: 'admin'
    }
}, {
    tableName: 'users'
});

const Client = sequelize.define('Client', {
    client_id: {
        type: DataTypes.STRING(255),
        allowNull: false,
        unique: true
    },
    alias: {
        type: DataTypes.STRING(100),
        allowNull: true
    },
    hostname: {
        type: DataTypes.STRING(255),
        allowNull: true
    },
    ip_address: {
        type: DataTypes.STRING(45), // IPv6 最大长度
        allowNull: true
    },
    platform: {
        type: DataTypes.STRING(50),
        allowNull: true
    },
    online_status: {
        type: DataTypes.BOOLEAN,
        defaultValue: false
    },
    last_seen: {
        type: DataTypes.DATE,
        allowNull: true
    }
}, {
    tableName: 'clients'
});

const CommandLog = sequelize.define('CommandLog', {
    client_id: {
        type: DataTypes.STRING(255),
        allowNull: false,
        references: {
            model: Client,
            key: 'client_id'
        }
    },
    command: {
        type: DataTypes.TEXT,
        allowNull: false
    },
    result: {
        type: DataTypes.TEXT,
        allowNull: true
    },
    exit_code: {
        type: DataTypes.INTEGER,
        allowNull: true
    },
    executed_at: {
        type: DataTypes.DATE,
        defaultValue: DataTypes.NOW
    }
}, {
    tableName: 'command_logs',
    timestamps: false
});

const Screenshot = sequelize.define('Screenshot', {
    client_id: {
        type: DataTypes.STRING(255),
        allowNull: false,
        references: {
            model: Client,
            key: 'client_id'
        }
    },
    filename: {
        type: DataTypes.STRING(255),
        allowNull: false
    },
    file_path: {
        type: DataTypes.STRING(500),
        allowNull: false
    },
    file_size: {
        type: DataTypes.INTEGER,
        allowNull: true
    },
    uploaded_at: {
        type: DataTypes.DATE,
        defaultValue: DataTypes.NOW
    }
}, {
    tableName: 'screenshots',
    timestamps: false
});

const UserSession = sequelize.define('UserSession', {
    session_id: {
        type: DataTypes.STRING(64),
        allowNull: false,
        unique: true
    },
    user_id: {
        type: DataTypes.INTEGER,
        allowNull: false,
        references: {
            model: User,
            key: 'id'
        }
    },
    expires_at: {
        type: DataTypes.DATE,
        allowNull: false
    },
    last_access: {
        type: DataTypes.DATE,
        defaultValue: DataTypes.NOW
    }
}, {
    tableName: 'user_sessions',
    timestamps: true,
    createdAt: 'created_at',
    updatedAt: false
});

const AuditLog = sequelize.define('AuditLog', {
    user_id: {
        type: DataTypes.INTEGER,
        allowNull: true,
        references: {
            model: User,
            key: 'id'
        }
    },
    action: {
        type: DataTypes.STRING(100),
        allowNull: false
    },
    target_client_id: {
        type: DataTypes.STRING(255),
        allowNull: true,
        references: {
            model: Client,
            key: 'client_id'
        }
    },
    details: {
        type: DataTypes.JSON,
        allowNull: true
    },
    ip_address: {
        type: DataTypes.STRING(45),
        allowNull: true
    }
}, {
    tableName: 'audit_logs',
    timestamps: true,
    createdAt: 'created_at',
    updatedAt: false
});

// 定义关联关系
User.hasMany(UserSession, { foreignKey: 'user_id' });
UserSession.belongsTo(User, { foreignKey: 'user_id' });

Client.hasMany(CommandLog, { foreignKey: 'client_id' });
CommandLog.belongsTo(Client, { foreignKey: 'client_id' });

Client.hasMany(Screenshot, { foreignKey: 'client_id' });
Screenshot.belongsTo(Client, { foreignKey: 'client_id' });

User.hasMany(AuditLog, { foreignKey: 'user_id' });
AuditLog.belongsTo(User, { foreignKey: 'user_id' });

Client.hasMany(AuditLog, { foreignKey: 'target_client_id' });
AuditLog.belongsTo(Client, { foreignKey: 'target_client_id' });

module.exports = {
    sequelize,
    User,
    Client,
    CommandLog,
    Screenshot,
    UserSession,
    AuditLog
};
