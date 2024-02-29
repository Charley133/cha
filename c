// SPDX-License-Identifier: MIT
pragma solidity ^0.8.0;

import "@openzeppelin/contracts/token/ERC20/IERC20.sol";
import "@openzeppelin/contracts/token/ERC20/utils/SafeERC20.sol";

contract YieldFarming {
    using SafeERC20 for IERC20;

    struct Pool {
        uint256 totalStaked;
        uint256 lastUpdateTime;
        uint256 rewardPerToken;
        uint256 totalRewards;
    }

    mapping(address => Pool) public pools;
    mapping(address => mapping(address => uint256)) public userStakes;
    mapping(address => uint256) public userRewards;

    IERC20 public stakingToken;
    IERC20 public rewardToken;
    uint256 public rewardRate; // Reward rate per second
    uint256 public totalStaked;

    event Staked(address indexed user, uint256 amount);
    event Withdrawn(address indexed user, uint256 amount);
    event RewardPaid(address indexed user, uint256 amount);

    constructor(address _stakingToken, address _rewardToken, uint256 _rewardRate) {
        stakingToken = IERC20(_stakingToken);
        rewardToken = IERC20(_rewardToken);
        rewardRate = _rewardRate;
    }

    function stake(uint256 amount) external {
        updateReward(msg.sender);
        require(amount > 0, "Amount must be greater than 0");
        stakingToken.safeTransferFrom(msg.sender, address(this), amount);
        pools[msg.sender].totalStaked += amount;
        totalStaked += amount;
        userStakes[msg.sender][address(stakingToken)] += amount;
        emit Staked(msg.sender, amount);
    }

    function withdraw(uint256 amount) external {
        updateReward(msg.sender);
        require(amount > 0, "Amount must be greater than 0");
        require(pools[msg.sender].totalStaked >= amount, "Insufficient staked amount");
        pools[msg.sender].totalStaked -= amount;
        totalStaked -= amount;
        userStakes[msg.sender][address(stakingToken)] -= amount;
        stakingToken.safeTransfer(msg.sender, amount);
        emit Withdrawn(msg.sender, amount);
    }

    function getReward() external {
        updateReward(msg.sender);
        uint256 reward = userRewards[msg.sender];
        userRewards[msg.sender] = 0;
        rewardToken.safeTransfer(msg.sender, reward);
        emit RewardPaid(msg.sender, reward);
    }

    function updateReward(address account) internal {
        Pool storage pool = pools[account];
        uint256 currentTime = block.timestamp;
        uint256 delta = currentTime - pool.lastUpdateTime;
        if (delta > 0 && pool.totalStaked > 0) {
            uint256 reward = (pool.totalStaked * delta * rewardRate) / 1e18;
            pool.rewardPerToken += (reward * 1e18) / pool.totalStaked;
            pool.lastUpdateTime = currentTime;
            pool.totalRewards += reward;
            userRewards[account] += (userStakes[account][address(stakingToken)] * (pool.rewardPerToken - 1e18)) / 1e18;
        }
    }
}
